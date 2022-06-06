/**************************************************************************************************
 * 
 *************************************************************************************************/

#ifndef HSM_H
#define HSM_H

/**************************************************************************************************
 * 
 *************************************************************************************************/
enum 
{
    HSM_NO_STATE_TRANSITION = -2,
    HSM_NULL_STATE          = -1,
    HSM_ROOT_STATE          = 0,
    HSM_USER_STATES_START   = 1
};

/**************************************************************************************************
 * 
 *************************************************************************************************/
enum
{
    EVENT_HSM_STATE_ENTRY = -3,
    EVENT_HSM_STATE_EXIT  = -2,
    EVENT_HSM_STATE_INIT  = -1,
    HSM_USER_EVENTS_START = 0
};

/**************************************************************************************************
 * This is done so that the HSM internals are not visible outside of the HSM implementation file.
 *************************************************************************************************/
struct Hsm;
typedef struct Hsm * Hsm_handle;

/**************************************************************************************************
 * The prototype for event handler functions that can be registered with the HSM.
 * 
 * @param   p_data      A pointer to data to relevent to the event. For example, you might have an
 *                      event like EVENT_PACKET_RECEIVED, and p_data could point to a buffer
 *                      containing the packet data.
 * 
 * @return  The state transition caused by the event handling. If no transition should occurs then
 *          the function should return HSM_NO_STATE_TRANSITION.
 *************************************************************************************************/
typedef int (*Event_handler)(void * p_data);

/**************************************************************************************************
 * Creates an HSM.
 * 
 * @param   num_states  The number of user states in the HSM (i.e. excluding the root state).
 * @param   num_events  The number of events recognised by the HSM.
 * 
 * @return  A handle to the newly created HSM. The handle can then be passed to the other API 
 *          functions to setup the state hierarchy, and event handlers.  
 *************************************************************************************************/
Hsm_handle hsm_create(int num_states, int num_events);

/**************************************************************************************************
 * Adds an event handler for an event to the specified state. Any unhandled events in a state 
 * (i.e. events that have not had a handler registered through a call to this function) will
 *************************************************************************************************/
void hsm_add_state_event_handler(Hsm_handle hsm, int state, int event, Event_handler handler);

/**************************************************************************************************
 * Adds a child to the specified state. This function is used to setup the state structure of the
 * HSM. starting from the HSM_ROOT_STATE, you start adding states, and sub-states (children), and
 * so on. 
 * 
 * @param   hsm     The HSM.
 * @param   state   The state that is to be the parent of the specified child (state).
 * @param   child   The state that is to be a child of the specified (parent) state.
 *************************************************************************************************/
void hsm_add_state_child(Hsm_handle hsm, int state, int child);

/**************************************************************************************************
 * Finalises the structure of an HSM. This must be called before the HSM starts handling events
 * through hsm_dispatch calls.
 * 
 * @param   hsm     The HSM.
 *************************************************************************************************/
void hsm_finalise_structure(Hsm_handle hsm);

/**************************************************************************************************
 * Dispatches an event, and data relevent to the event, to the HSM for handling. 
 * TODO: add more documentation about what happens internally.
 * 
 * @param   event   The event ID
 * @param   p_data  A pointer to the event data.
 *************************************************************************************************/
void hsm_dispatch(Hsm_handle hsm, int event, void * p_data);

#endif // HSM_H
