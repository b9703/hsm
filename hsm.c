/**************************************************************************************************
 * 
 * 
 *************************************************************************************************/


#include "hsm.h"
#include <malloc.h>
#include <assert.h>
#include <stdbool.h>

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
    Event_handler * p_event_handlers;
    int * p_dir_map;
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

/**************************************************************************************************
 * 
 *************************************************************************************************/
static struct State * alloc_states(int num_states, int num_events)
{
    struct State * p_states = (struct State *)malloc(sizeof(struct State)*num_states);

    for (int i = 0; i < num_states; i++)
    {
        p_states[i].parent = HSM_NULL_STATE;
        p_states[i].p_children = (int *)malloc(sizeof(int)*num_states);
        p_states[i].num_children = 0;
        p_states[i].p_event_handlers = (Event_handler *)malloc(sizeof(Event_handler)*num_events);
        p_states[i].p_dir_map     = (int *)malloc(sizeof(int)*num_states);

        for (int j = 0; j < num_events; j++) { p_states[i].p_event_handlers[j] = NULL; }
        for (int j = 0; j < num_states; j++) { p_states[i].p_children[j] = HSM_NULL_STATE; }
    }

    return p_states;
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
    hsm->p_states[state].p_event_handlers[event] = handler;
}

/**************************************************************************************************
 * 
 *************************************************************************************************/
static Event_handler state_get_event_handler(Hsm_handle hsm, int state, int event)
{
    return hsm->p_states[state].p_event_handlers[event];
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
static void flatten_handlers(Hsm_handle hsm)
{
    for (int s = 0; s < hsm->num_states; s++)
    {
        for (int e = 0; e < hsm->num_events; e++)
        {
            int curr_s = s;
            Event_handler h = NULL;
            do
            {
                h = state_get_event_handler(hsm, curr_s, e);
                curr_s = state_get_parent(hsm, curr_s);
            } while ((h == NULL) && (curr_s != HSM_NULL_STATE));
            
            state_set_event_handler(hsm, s, e, h);
        }
    }
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
static int handle_event(Hsm_handle hsm, int event, void * p_data)
{
    struct State * p_curr_state = &hsm->p_states[hsm->curr_state];

    Event_handler p_handler = p_curr_state->p_event_handlers[event];
    if (p_handler == NULL)
    {
        return HSM_NO_STATE_TRANSITION; 
    }

    int new_state = (*p_handler)(p_data);

    return new_state;
}

/**************************************************************************************************
 * 
 *************************************************************************************************/
static void transition_to_state(Hsm_handle hsm, int state)
{
    int destination_state = state;
    while (destination_state != HSM_NO_STATE_TRANSITION)
    {
        while (true)
        {
            struct State * p_curr_state = &hsm->p_states[hsm->curr_state];

            /* Get the dir(ection) of the next step towards the destination state. 
             * The possibilities for a valid HSM are: go to parent, go to one of the
             * current state's children, or stop because the current state is the 
             * destination state.
             */
            int dir = p_curr_state->p_dir_map[state];
            assert((dir != DIR_UNKNOWN) && (dir != DIR_UNREACHABLE));
            if (dir == DIR_STOP)
            {
                assert(hsm->curr_state == destination_state);
                break;
            }
            
            // Exit the current state.
            int ret = handle_event(hsm, EVENT_HSM_STATE_EXIT, NULL);
            assert(ret == HSM_NO_STATE_TRANSITION);

            // Enter the new state.
            if (dir == DIR_PARENT)
            {
                // Go to parent and do not execute entry action.
                hsm->curr_state = p_curr_state->parent;
            }
            else if (dir >= 0)
            {
                // Go to child and execute entry action.
                hsm->curr_state = p_curr_state->p_children[dir];
                ret = handle_event(hsm, EVENT_HSM_STATE_ENTRY, NULL);
                assert(ret == HSM_NO_STATE_TRANSITION);
            }
        }

        /* Unlike entry and exit events, init events not only can, but always do cause state
         * transitions. Only if the current state does not have a state init handler, will the
         * this loop exit.
         */
        destination_state = handle_event(hsm, EVENT_HSM_STATE_INIT, NULL);
    }
}

/**************************************************************************************************
 * HSM API functions
 *************************************************************************************************/
Hsm_handle hsm_create(int num_states, int num_events)
{
    Hsm_handle hsm = (Hsm_handle)malloc(sizeof(struct Hsm));

    hsm->p_states   = alloc_states(num_states, num_events);
    hsm->num_events = num_events;
    hsm->num_states = num_states;
    hsm->curr_state = HSM_ROOT_STATE;
    hsm->structure_is_finalised      = false;

    return hsm;
}

void hsm_add_state_event_handler(Hsm_handle hsm, int state, int event, Event_handler handler)
{
    assert(!hsm->structure_is_finalised);
    state_set_event_handler(hsm, state, event, handler);
}

void hsm_add_state_child(Hsm_handle hsm, int state, int child)
{
    assert(!hsm->structure_is_finalised);
    state_add_child(hsm, state, child);
    state_set_parent(hsm, child, state);
}

void hsm_finalise_structure(Hsm_handle hsm)
{
    assert(!hsm->structure_is_finalised);
    flatten_handlers(hsm);
    calc_transition_dirs(hsm);
    hsm->structure_is_finalised = true;
}

void hsm_dispatch(Hsm_handle hsm, int event, void * p_data)
{
    assert(hsm->structure_is_finalised);
    assert(event >= HSM_USER_EVENTS_START);

    // Handle the event
    int new_state = handle_event(hsm, event, p_data);

    // Handle resultant state transitions.
    if (new_state != HSM_NO_STATE_TRANSITION)
    {
        transition_to_state(hsm, new_state);
    }
}
