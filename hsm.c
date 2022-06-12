/**************************************************************************************************
 * 
 * Say we are in a state s, and we are handling an event e. What are the possibilities?
 * 1. s handles e and transitions to a state that is not a direct ancestor.
 * 2. s handles e and transitions to itself. Just do the exit and then entry action followed by 
 *    and init handling and so on.
 * 3. s handles e and transitions to a state that is a direct ancestor. In this case we need to 
 *    know if its a local or external transition. Either way we go up to the target state. If its
 *    a local trans then we do not execute the exit then entry for the target, we just start from
 *    the init action if there is one. If its an external trans then we must do the exit and entry
 *    actions before the doing the init.
 * 4. e is handled by an ancestor of s. Again it could be local or external.
 * 
 * Are there an invalid transitions? If theres an init handler can a transition go directly to a
 * substate?
 * 
 * 
 *************************************************************************************************/

/* TODO: 
 * - make sure there are no problems not caught by asserts. As the basis for an application,
 *   the HSM must be absolutely bullet proof.
 * - Add the option for tracing and profiling e.g. event handling
 * - Add support for event deferral.
 * - Detect malformed state machines.
 * -
 *
 * 
 */


#include "hsm.h"
#include <malloc.h>
#include <assert.h>
#include <stdbool.h>

#define TRANS_EQUAL(t1, t2)   (((t1.state) == (t2.state)) && ((t1.type) == (t2.type)))

/**************************************************************************************************
 * 
 *************************************************************************************************/
enum
{
    DIR_UNKNOWN     = -4,
    DIR_UNREACHABLE = -3,
    DIR_PARENT      = -2,
    DIR_STOP        = -1
};

/**************************************************************************************************
 * 
 *************************************************************************************************/
struct State
{
    int parent;
    int * p_children;
    int num_children;
    int * p_dir_map;

    Event_handler * p_user_event_handlers;
    Event_handler entry_handler;
    Event_handler exit_handler;
    Event_handler init_handler;
};

/**************************************************************************************************
 * 
 *************************************************************************************************/
struct Hsm
{
    struct State * p_states;

    int num_states;
    int num_events;
    int curr_state;

    bool structure_is_finalised;
};

struct Trans_info
{
    struct Hsm_trans trans;
    int trans_source_state;

};

/**************************************************************************************************
 * 
 *************************************************************************************************/
static struct State * alloc_states(int num_states, int num_events)
{
    struct State * p_states = (struct State *)malloc(sizeof(struct State)*num_states);

    for (int i = 0; i < num_states; i++)
    {
        p_states[i].parent = HSM_STATE_NULL;
        p_states[i].p_children = (int *)malloc(sizeof(int)*num_states);
        p_states[i].num_children = 0;
        p_states[i].p_user_event_handlers = (Event_handler *)malloc(sizeof(Event_handler)*num_events);
        p_states[i].p_dir_map     = (int *)malloc(sizeof(int)*num_states);

        for (int j = 0; j < num_events; j++) { p_states[i].p_user_event_handlers [j] = NULL; }
        for (int j = 0; j < num_states; j++) { p_states[i].p_children[j] = HSM_STATE_NULL; }
    }

    return p_states;
}

void dealloc_states(Hsm_handle hsm)
{
    for (int i = 0; i < hsm->num_states; i++)
    {
        free(hsm->p_states[i].p_children);
        free(hsm->p_states[i].p_user_event_handlers);
        free(hsm->p_states[i].p_dir_map);
    }

    free(hsm->p_states);
}

/**************************************************************************************************
 * 
 *************************************************************************************************/
static void state_add_child(Hsm_handle hsm, int state, int child)
{
    struct State * p_state = &hsm->p_states[state];
    p_state->p_children[p_state->num_children++] = child;
}

/**************************************************************************************************
 * 
 *************************************************************************************************/
static void state_set_parent(Hsm_handle hsm, int state, int parent)
{
    hsm->p_states[state].parent = parent;
}

/**************************************************************************************************
 * 
 *************************************************************************************************/
static void state_set_event_handler(Hsm_handle hsm, int state, int event, Event_handler handler)
{
    struct State * s = &hsm->p_states[state];
    if      (event == HSM_EVENT_STATE_ENTRY) { s->entry_handler = handler; }
    else if (event == HSM_EVENT_STATE_EXIT)  { s->exit_handler  = handler; }
    else if (event == HSM_EVENT_STATE_INIT)  { s->init_handler  = handler; }
    else if (event >= HSM_USER_EVENTS_START) { s->p_user_event_handlers[event] = handler; }
    else { assert(false); }
}

/**************************************************************************************************
 * 
 *************************************************************************************************/
static int state_get_parent(Hsm_handle hsm, int state)
{
    return hsm->p_states[state].parent;
}

/**************************************************************************************************
 * 
 *************************************************************************************************/
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


/**************************************************************************************************
 * 
 *************************************************************************************************/
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
                dir = DIR_STOP;
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

/**************************************************************************************************
 * 
 *************************************************************************************************/
static struct Trans_info handle_user_event(Hsm_handle hsm, int event, void * p_data)
{

    Event_handler p_handler = NULL;

    int handler_state = hsm->curr_state;
    while (handler_state != HSM_STATE_NULL) 
    {
        Event_handler p_handler = hsm->p_states[handler_state].p_user_event_handlers[event];
        if (p_handler != NULL)
        {
            break;
        }

        handler_state = state_get_parent(hsm, handler_state);
    }


    struct Trans_info ti = {
        .trans = HSM_TRANS_NONE(),
        .trans_source_state = HSM_STATE_NULL
    };

    // Event was unhandled at all levels so nothing to do.
    if (p_handler != NULL)
    {
        ti.trans = (*p_handler)(p_data),
        ti.trans_source_state = handler_state;
    }

    return ti;
}

static void do_exit_action(Hsm_handle hsm, int state)
{
    Event_handler exit_handler = hsm->p_states[state].exit_handler;
    if (exit_handler != NULL)
    {
        struct Hsm_trans trans = (*exit_handler)(NULL);
        assert(trans.target == HSM_STATE_NULL);
    }

    
}

static void do_entry_action(Hsm_handle hsm, int state)
{
    Event_handler entry_handler = hsm->p_states[state].entry_handler;
    if (entry_handler != NULL)
    {
        struct Hsm_trans trans = (*entry_handler)(NULL);
        assert(trans.target == HSM_STATE_NULL);
    }
}

static int do_init_action(Hsm_handle hsm, int state)
{
    Event_handler init_handler = hsm->p_states[state].init_handler;
    if (init_handler == NULL)
    {
        return HSM_STATE_NULL;
    }

    struct Hsm_trans trans = (*init_handler)(NULL);
    assert(trans.target != HSM_STATE_NULL);
    return trans.target;
}

static void handle_transition(Hsm_handle hsm, struct Trans_info ti)
{
    if (ti.trans.target == HSM_STATE_NULL)
    {
        return;
    }

    int cs  = hsm->curr_state;
    int ts  = ti.trans.target;

    /* Firstly, go out to the state that was the origin of the transition. This will either be the
     * current state, or one of its ancestors.
     */
    while (cs != ti.trans_source_state)
    {
        do_exit_action(hsm, cs);
        cs = hsm->p_states[cs].parent;
    }

    /* From here, there are a three possibilities:
     * 1. the target state is the transition source state. In this case the transition is a self
     *    transition.
     * 2. the target state is contained by the transition source state. In this case we need to
     *    know whether the transition is a local one, or an external one. If its a local one then
     *    we just drill down to the target state. If its an external one then we first need to do
     *    the exit then entry actions for the transition source state before drilling down to the
     *    target state.
     * 3. the target state is not contained by the transition source state. In this case we 
     *    continue outwards to the LCA of the two states and then drill down to the target state.
     *    NOTE: this case is not handled by the following if-else, but by the final loop.
     */
    int dir = hsm->p_states[cs].p_dir_map[ts];
    if (cs == ts)
    {
        do_exit_action(hsm, cs);
        do_entry_action(hsm, cs);
    }
    else if (dir >= 0)
    {
        if (ti.trans.type != HSM_TRANS_TYPE_LOCAL)
        {
            do_exit_action(hsm, cs);
            do_entry_action(hsm, cs);
        }
    }

    /* Finally, we do the rest of the transition. 
     *
     */
    while (ts != HSM_STATE_NULL)
    {
        if (cs == ts)
        {
            ts = do_init_action(hsm, cs);
        }
        else
        {
            int dir = hsm->p_states[cs].p_dir_map[ts];
            if (dir >= 0)
            {
                cs = hsm->p_states[cs].p_children[dir];
                do_entry_action(hsm, cs);
            }
            else if (dir == DIR_PARENT)
            {
                do_exit_action(hsm, cs);
                cs = hsm->p_states[cs].parent;
            }
            else { assert(false); }
        }
    }

    
}

/**************************************************************************************************
 * HSM API functions
 *************************************************************************************************/
Hsm_handle hsm_create(int num_states, int num_events)
{
    /* TODO: maybe add the option for a custom allocator. 
     * TODO: handle allocation fail. Maybe assert since this should only be done at init time and
     *       failure cannot really be recovered from. 
     * TODO: maybe add the option for a custom assert.
     */
    Hsm_handle hsm = (Hsm_handle)malloc(sizeof(struct Hsm));

    hsm->p_states   = alloc_states(num_states, num_events);
    hsm->num_events = num_events;
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

void hsm_state_set_event_handlers(Hsm_handle hsm,
                                  struct Hsm_event_handler_pair * p_eh_pairs,
                                  int num_eh_pairs)
{

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

    int cs = hsm->curr_state;
    int ts = do_init_action(hsm, cs);
    assert(ts != HSM_STATE_NULL);
    while (ts != HSM_STATE_NULL)
    {
        if (cs == ts)
        {
            ts = do_init_action(hsm, cs);
        }
        else
        {
            int dir = hsm->p_states[cs].p_dir_map[ts];
            if (dir >= 0)
            {
                cs = hsm->p_states[cs].p_children[dir];
                do_entry_action(hsm, cs);
            }
            else if (dir == DIR_PARENT)
            {
                do_exit_action(hsm, cs);
                cs = hsm->p_states[cs].parent;
            }
            else { assert(false); }
        }
    }

    hsm->structure_is_finalised = true;
}

void hsm_dispatch(Hsm_handle hsm, int event, void * p_data)
{
    assert(hsm->structure_is_finalised);
    assert(event >= HSM_USER_EVENTS_START);

    struct Trans_info ti = handle_user_event(hsm, event, p_data);

    handle_transition(hsm, ti);
}


void hsm_set_state(Hsm_handle hsm, int state)
{
#ifdef HSM_TEST
    hsm->curr_state = state;
#else
    assert(false);
#endif
}
