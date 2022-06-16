/**************************************************************************************************
 * 
 *************************************************************************************************/



#ifndef HSM_H
#define HSM_H

enum Hsm_trans_type
{
    HSM_TRANS_TYPE_LOCAL,
    HSM_TRANS_TYPE_EXTERNAL,
    HSM_TRANS_TYPE_DEFAULT
};

struct Hsm_trans
{
    int target;
    enum Hsm_trans_type type;
};

/**************************************************************************************************
 * The prototype for event handler functions that can be registered with the HSM.
 * 
 * @param   p_data      A pointer to data to relevent to the event. For example, you might have an
 *                      event like EVENT_PACKET_RECEIVED, and p_data could point to a buffer
 *                      containing the packet data.
 * 
 * @return  The state transition caused by the event handling. 
 *************************************************************************************************/
typedef struct Hsm_trans (*Event_handler)(void * p_data);

struct Hsm_event_handler_pair
{
    int event;
    Event_handler handler;
};

/**************************************************************************************************
 * 
 *************************************************************************************************/
enum 
{
    HSM_STATE_NULL = -1,
    HSM_STATE_ROOT = 0,
    HSM_USER_STATES_START = 1
};

/**************************************************************************************************
 * 
 *************************************************************************************************/
enum
{
    HSM_EVENT_STATE_ENTRY = -3,
    HSM_EVENT_STATE_EXIT  = -2,
    HSM_EVENT_STATE_INIT  = -1,
    HSM_USER_EVENTS_START = 0
};


/*
 *
 */
#define HSM_TRANS(s) (struct Hsm_trans){ .target=(s), .type=HSM_TRANS_TYPE_DEFAULT }

/*
 *
 */
#define HSM_TRANS_LOCAL(s)     (struct Hsm_trans){ .target=(s), .type=HSM_TRANS_TYPE_LOCAL    }
#define HSM_TRANS_EXTERNAL(s)  (struct Hsm_trans){ .target=(s), .type=HSM_TRANS_TYPE_EXTERNAL }

/* 
 *
 */
#define HSM_TRANS_NONE()  (struct Hsm_trans){ .target=HSM_STATE_NULL, .type=HSM_TRANS_TYPE_DEFAULT }

/**************************************************************************************************
 * This is done so that the HSM internals are not visible outside of the HSM implementation file.
 *************************************************************************************************/
struct Hsm;
typedef struct Hsm * Hsm_handle;

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

void hsm_destroy(Hsm_handle hsm);

/**************************************************************************************************
 * Adds an event handler for an event to the specified state. Any unhandled events in a state 
 * (i.e. events that have not had a handler registered through a call to this function) will
 *************************************************************************************************/
void hsm_state_set_event_handler(Hsm_handle hsm, int state, int event, Event_handler handler);

void hsm_state_set_event_handlers(Hsm_handle hsm,
                                  struct Hsm_event_handler_pair * p_eh_pairs,
                                  int num_eh_pairs);

/**************************************************************************************************
 * Adds a child to the specified state. This function is used to setup the state structure of the
 * HSM. starting from the HSM_ROOT_STATE, you start adding states, and sub-states (children), and
 * so on. 
 * 
 * @param   hsm     The HSM.
 * @param   state   The state that is to be the parent of the specified child (state).
 * @param   child   The state that is to be a child of the specified (parent) state.
 *************************************************************************************************/
void hsm_state_add_child(Hsm_handle hsm, int state, int child);

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

/**************************************************************************************************
 * Used for setting the HSM state from the outside. Note: this is intended only for testing and so
 * it will trigger an assertion if called in a test build (HSM_TEST is defined).
 * 
 * @param   hsm     The HSM.
 * @param   state   The state to set. 
 *************************************************************************************************/
void hsm_set_state(Hsm_handle hsm, int state);

#endif // HSM_H
