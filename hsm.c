/**************************************************************************************************
 * 
 * 
 *************************************************************************************************/

/* TODO: 
 * - make sure there are no problems not caught by asserts. As the basis for an application,
 *   the HSM must be absolutely bullet proof.
 * - Add the option for tracing and profiling e.g. event handling
 * - Add support for event deferral.
 * - Detect malformed state machines.
 * - Must a containing state have an init handler? probably.
 * - Provide the option for custom allocator and assert.
 *
 * 
 */


#include "hsm.h"
#include <malloc.h>
#include <assert.h>
#include <stdbool.h>


// TODO:
enum
{
    DIR_UNKNOWN     = -4, // Default value.
    DIR_UNREACHABLE = -3, // State cannot be reached (indicates an invalid HSM).
    DIR_PARENT      = -2, // Go towards the parent.
    DIR_ARRIVED     = -1  // Stop since already at the target state.
};

// A struct containing all of the data needed by the HSM for a particular state.
struct State
{
    int parent;        // The ID of the state's parent/containing state
    int * p_children;  // An array of IDs of the state's children/sub-states
    int num_children;  // The number of direct substates (children) the state has
    int * p_dir_map;   /* Tells the HSM which direction to go in order to transition to a 
                        * particular state.
                        */

    Event_handler * p_user_event_handlers; // An array of user event handlers.
    Event_handler entry_handler;           // The entry event handler.
    Event_handler exit_handler;            // The exit event handler.
    Event_handler init_handler;            // The init event handler.
};

// All the data for an HSM.
struct Hsm
{
    struct State * p_states; // An array of the states used by the HSM.
    int num_states;          // Number of states in the HSM, including the root state.
    int num_user_events;     // Number of different types of user event.
    int curr_state;          // The current state that the HSM is in.

    bool structure_is_finalised;
};

/*
 *
 */
struct Event_handler_state_pair
{
    Event_handler handler;
    int source_state;
};


/**************************************************************************************************
 * Static functions
 *************************************************************************************************/
static struct State * alloc_states(int num_states, int num_user_events);
static void dealloc_states(Hsm_handle hsm);
static void state_add_child(Hsm_handle hsm, int state, int child);
static void state_set_parent(Hsm_handle hsm, int state, int parent);
static void state_set_event_handler(Hsm_handle hsm, int state, int event, Event_handler handler);
static int state_get_parent(Hsm_handle hsm, int state);
static int dest_below(Hsm_handle hsm, int s, int d);
static void calc_transition_dirs(Hsm_handle hsm);
static struct Event_handler_state_pair get_user_event_handler(Hsm_handle hsm, int event);
static void do_entry_action(Hsm_handle hsm);
static void do_exit_action(Hsm_handle hsm);
static void do_init_actions(Hsm_handle hsm);
static void transition_to_state(Hsm_handle hsm, int target);
static bool is_self_transition(Hsm_handle const hsm, int target);
static bool is_sub_state_transition(Hsm_handle const hsm, int target);
static void handle_user_event(Hsm_handle hsm, int event, void * p_data);


/**************************************************************************************************
 * HSM API functions
 *************************************************************************************************/
Hsm_handle hsm_create(int num_states, int num_user_events)
{
    /* TODO: maybe add the option for a custom allocator. 
     * TODO: handle allocation fail. Maybe assert since this should only be done at init time and
     *       failure cannot really be recovered from. 
     * TODO: maybe add the option for a custom assert.
     */
    Hsm_handle hsm = (Hsm_handle)malloc(sizeof(struct Hsm));

    hsm->p_states   = alloc_states(num_states, num_user_events);
    hsm->num_user_events = num_user_events;
    hsm->num_states = num_states;
    hsm->curr_state = HSM_STATE_ROOT;
    hsm->structure_is_finalised = false;

    return hsm;
}

void hsm_destroy(Hsm_handle hsm)
{
    dealloc_states(hsm);
    free(hsm);
}

void hsm_state_set_event_handler(Hsm_handle hsm, int state, int event, Event_handler handler)
{
    assert(!hsm->structure_is_finalised);
    state_set_event_handler(hsm, state, event, handler);
}

void hsm_state_set_event_handler_table(Hsm_handle hsm,
                                      int state,
                                      struct Hsm_event_handler_pair const * const p_eh_pairs,
                                      int num_eh_pairs)
{
    assert(!hsm->structure_is_finalised);
    for (int i = 0; i < num_eh_pairs; i++)
    {
        state_set_event_handler(hsm, state, p_eh_pairs[i].event, p_eh_pairs[i].handler);
    }
}

void hsm_state_add_child(Hsm_handle hsm, int state, int child)
{
    assert(!hsm->structure_is_finalised);
    state_add_child(hsm, state, child);
    state_set_parent(hsm, child, state);
}

void hsm_finalise_structure(Hsm_handle hsm)
{
    assert(!hsm->structure_is_finalised);

    calc_transition_dirs(hsm);

    do_init_actions(hsm);

    hsm->structure_is_finalised = true;
}

void hsm_dispatch(Hsm_handle hsm, int event, void * p_data)
{
    assert(hsm->structure_is_finalised);
    assert(event >= HSM_USER_EVENTS_START);

    handle_user_event(hsm, event, p_data);
}


void hsm_set_state(Hsm_handle hsm, int state)
{
#ifdef HSM_TEST
    hsm->curr_state = state;
#else
    assert(false);
#endif
}

/**************************************************************************************************
 * Static function definitions
 *************************************************************************************************/
static struct State * alloc_states(int num_states, int num_user_events)
{
    struct State * p_states = (struct State *)malloc(sizeof(struct State)*num_states);

    for (int i = 0; i < num_states; i++)
    {
        p_states[i].parent = HSM_STATE_NULL;
        p_states[i].p_children = (int *)malloc(sizeof(int)*num_states);
        p_states[i].num_children = 0;
        p_states[i].p_user_event_handlers = (Event_handler *)malloc(sizeof(Event_handler)*num_user_events);
        p_states[i].p_dir_map     = (int *)malloc(sizeof(int)*num_states);

        for (int j = 0; j < num_user_events; j++) { p_states[i].p_user_event_handlers [j] = NULL; }
        for (int j = 0; j < num_states; j++) { p_states[i].p_children[j] = HSM_STATE_NULL; }
    }

    return p_states;
}

static void dealloc_states(Hsm_handle hsm)
{
    for (int i = 0; i < hsm->num_states; i++)
    {
        free(hsm->p_states[i].p_children);
        free(hsm->p_states[i].p_user_event_handlers);
        free(hsm->p_states[i].p_dir_map);
    }

    free(hsm->p_states);
}


static void state_add_child(Hsm_handle hsm, int state, int child)
{
    struct State * p_state = &hsm->p_states[state];
    p_state->p_children[p_state->num_children++] = child;
}

static void state_set_parent(Hsm_handle hsm, int state, int parent)
{
    hsm->p_states[state].parent = parent;
}

static void state_set_event_handler(Hsm_handle hsm, int state, int event, Event_handler handler)
{
    struct State * s = &hsm->p_states[state];
    if      (event == HSM_EVENT_STATE_ENTRY) { s->entry_handler = handler; }
    else if (event == HSM_EVENT_STATE_EXIT)  { s->exit_handler  = handler; }
    else if (event == HSM_EVENT_STATE_INIT)  { s->init_handler  = handler; }
    else if (event >= HSM_USER_EVENTS_START) { s->p_user_event_handlers[event] = handler; }
    else { assert(false); }
}

static int state_get_parent(Hsm_handle hsm, int state)
{
    return hsm->p_states[state].parent;
}

static int dest_below(Hsm_handle hsm, int s, int d)
{
    for (int c = 0; c < hsm->p_states[s].num_children; c++)
    {
        if ((hsm->p_states[s].p_children[c] == d) 
        ||  (dest_below(hsm, hsm->p_states[s].p_children[c], d) != -1))
        {
            return c;
        }
    }

    return -1;
}

static void calc_transition_dirs(Hsm_handle hsm)
{
    /* The root state should have all other states underneath it somewhere in the hierarchy.
     * If no state fits this condition then the state machine is not connected properly.
     */
    bool root_state_found = false;
    for (int s = 0; s < hsm->num_states; s++)
    {
        bool is_root_state = true;
        for (int d = 0; d < hsm->num_states; d++)
        {
            int dir = DIR_PARENT; // Assume parent.
            if (d == s)
            {
                dir = DIR_ARRIVED;
            }
            else 
            {
                int c = dest_below(hsm, s, d);
                if (c != -1)
                {
                    dir = c;
                }
            }

            hsm->p_states[s].p_dir_map[d] = dir;

            // Root state has all other states below it.
            if (dir == DIR_PARENT)
            {
                is_root_state = false;
            }
        }
     
        if (is_root_state)
        {
            root_state_found = true;
        }
    }

    // There must be a root state otherwise the HSM states are not connected properly.
    assert(root_state_found);
}

static struct Event_handler_state_pair get_user_event_handler(Hsm_handle hsm, int event)
{
    assert(event >= HSM_USER_EVENTS_START);

    struct Event_handler_state_pair ehsp = {
        .handler = NULL,
        .source_state = HSM_STATE_NULL
    };

    int handler_state = hsm->curr_state;
    while (handler_state != HSM_STATE_NULL) 
    {
        ehsp.handler = hsm->p_states[handler_state].p_user_event_handlers[event];
        if (ehsp.handler != NULL)
        {
            ehsp.source_state = handler_state;
            break;
        }

        handler_state = state_get_parent(hsm, handler_state);
    }

    return ehsp;
}

static void do_entry_action(Hsm_handle hsm)
{
    Event_handler handler = hsm->p_states[hsm->curr_state].entry_handler;
    if (handler != NULL)
    {
        struct Hsm_trans trans = (*handler)(NULL);
        assert(trans.target == HSM_STATE_NULL);
    }
}

static void do_exit_action(Hsm_handle hsm)
{
    Event_handler handler = hsm->p_states[hsm->curr_state].exit_handler;
    if (handler != NULL)
    {
        struct Hsm_trans trans = (*handler)(NULL);
        assert(trans.target == HSM_STATE_NULL);
    }
}

static void do_init_actions(Hsm_handle hsm)
{
    while (true)
    {
        Event_handler handler = hsm->p_states[hsm->curr_state].init_handler;
        if (handler == NULL)
        {
            // TODO: assert in a leaf state.
            break;
        }

        struct Hsm_trans trans = (*handler)(NULL);
        if (trans.target == HSM_STATE_NULL)
        {
            break;
        }
        
        // TODO: assert target is a child of the current state.
        hsm->curr_state = trans.target;
        do_entry_action(hsm);
    }
}

static void transition_to_state(Hsm_handle hsm, int target)
{
    while (hsm->curr_state != target)
    {
        int dir = hsm->p_states[hsm->curr_state].p_dir_map[target];
        if (dir >= 0)
        {
            hsm->curr_state = hsm->p_states[hsm->curr_state].p_children[dir];
            do_entry_action(hsm);
        }
        else if (dir == DIR_PARENT)
        {
            do_exit_action(hsm);
            hsm->curr_state = hsm->p_states[hsm->curr_state].parent;
        }
        else 
        {
            assert(false); 
        }
    }
}

static bool is_self_transition(Hsm_handle const hsm, int target)
{
    return (target == hsm->curr_state);
   
}

static bool is_sub_state_transition(Hsm_handle const hsm, int target)
{
     int dir = hsm->p_states[hsm->curr_state].p_dir_map[target];
     return (dir >= 0);
}

static void handle_user_event(Hsm_handle hsm, int event, void * p_data)
{
    struct Event_handler_state_pair ehsp = get_user_event_handler(hsm, event);
    if (ehsp.handler == NULL)
    {
        return;
    }

    // Execute the event handler.
    struct Hsm_trans trans = (*ehsp.handler)(p_data);
    if (trans.target == HSM_STATE_NULL)
    {
        return;
    }
    
    /* If the source of the event handling is not the current state then the event was handled
     * by one of the current states ancestors. If this is the case then we must first transition
     * up to that state.
     */
    if (ehsp.source_state != hsm->curr_state)
    {
        transition_to_state(hsm, ehsp.source_state);
    }

    // At this point we are at the state that produced the transition (i.e. handled the event)
    if (is_self_transition(hsm, trans.target))
    {
        do_exit_action(hsm);
        do_entry_action(hsm);
    }
    else if (is_sub_state_transition(hsm, trans.target))
    {
        if (trans.type == HSM_TRANS_TYPE_EXTERNAL)
        {
            do_exit_action(hsm);
            do_entry_action(hsm);
        }

        transition_to_state(hsm, trans.target);
    }
    else
    {
        transition_to_state(hsm, trans.target);

        // TODO: check that its a direct ancestor.
        if (trans.type == HSM_TRANS_TYPE_EXTERNAL)
        {
            do_exit_action(hsm);
            do_entry_action(hsm);
        }
    }

    do_init_actions(hsm);
}
