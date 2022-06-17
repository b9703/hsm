/**************************************************************************************************
 * Usage:
 * 1. Define the states in the state machine. For example...
 *        
 *    enum State
 *    {
 *        STATE_S0 = HSM_USER_STATES_START,
 *        STATE_S1,
 *        STATE_S2,
 *    };
 * 
 * 2. Define the user types of user events that the state machine must handle. For example...
 *    
 *    enum Event
 *    {
 *        EVENT_E0 = HSM_USER_EVENTS_START,
 *        EVENT_E1,
 *        EVENT_E2,
 *    };
 * 
 * 3. Write event handlers for the states based on your state machine diagram. 
 * 
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

// An HSM trans(ition).
struct Hsm_trans
{
    int target;               // The target state of the transition.
    enum Hsm_trans_type type; // The transition type.
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

// Used for constructing event handler tables.
struct Hsm_event_handler_pair
{
    int event;
    Event_handler handler;
};

#define HANDLER_TABLE_SIZE(table)   (int)(sizeof((table))/sizeof((table[0])))

// HSM built-in state values.
enum 
{
    HSM_STATE_NULL = -1,       // Indicates no state (used to specify no transition).
    HSM_STATE_ROOT = 0,        // The root state must have the ID 0.
    HSM_USER_STATES_START = 1  // User states must start from here.
};

// HSM built-in events.
enum
{
    HSM_EVENT_STATE_ENTRY = -3,
    HSM_EVENT_STATE_EXIT  = -2,
    HSM_EVENT_STATE_INIT  = -1,
    HSM_USER_EVENTS_START = 0
};


/* Used for most transitions. Ones where it is not necessary to specify whether or not the 
 * transition is local or external.
 */
#define HSM_TRANS(s) (struct Hsm_trans){ .target=(s), .type=HSM_TRANS_TYPE_DEFAULT }

/*
 *
 */
#define HSM_TRANS_LOCAL(s)     (struct Hsm_trans){ .target=(s), .type=HSM_TRANS_TYPE_LOCAL    }

/*
 *
 */
#define HSM_TRANS_EXTERNAL(s)  (struct Hsm_trans){ .target=(s), .type=HSM_TRANS_TYPE_EXTERNAL }

// Should be returned by event handlers when no state transition is required.
#define HSM_TRANS_NONE()  (struct Hsm_trans){ .target=HSM_STATE_NULL, .type=HSM_TRANS_TYPE_DEFAULT }

/**************************************************************************************************
 * This is done so that the HSM internals are not visible outside of the HSM implementation file.
 *************************************************************************************************/
struct Hsm;
typedef struct Hsm * Hsm_handle;

/**************************************************************************************************
 * Creates an HSM.
 * 
 * @param   num_user_states  The number of user states in the HSM (i.e. excluding the root state).
 * @param   num_user_events  The number of different event types that need to be handled by the
 *                           HSM, excluding HSM built-in events such as entry, exit, and init.
 * 
 * @return  A handle to the newly created HSM. The handle can then be passed to the other API 
 *          functions to setup the state hierarchy, and event handlers.  
 *************************************************************************************************/
Hsm_handle hsm_create(int num_user_states, int num_user_events);

void hsm_destroy(Hsm_handle hsm);

/**************************************************************************************************
 * Adds an event handler for an event to the specified state. Any unhandled events in a state 
 * (i.e. events that have not had a handler registered through a call to this function) will
 *************************************************************************************************/
void hsm_state_set_event_handler(Hsm_handle hsm, int state, int event, Event_handler handler);

void hsm_state_set_event_handler_table(Hsm_handle hsm,
                                       int state,
                                       struct Hsm_event_handler_pair const * const p_eh_pairs,
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
 * @param   hsm     The HSM
 * @param   event   The event ID
 * @param   p_data  A pointer to the event data. The event handler should know the type of data it
 *                  is receiving and cast the pointer to that type.
 *************************************************************************************************/
void hsm_dispatch(Hsm_handle hsm, int event, void * p_data);

/**************************************************************************************************
 * Used for setting the HSM state from the outside. 
 * 
 * @note    this is intended only for testing and so it will trigger an assertion if called outside
 *          of a test build (i.e. HSM_TEST not defined). To use in a test build, add -DHSM_TEST to
 *          the compilation (TODO: is this supported by other compilers?).
 * 
 * @param   hsm     The HSM.
 * @param   state   The state to set. 
 *************************************************************************************************/
void hsm_set_state(Hsm_handle hsm, int state);

#endif // HSM_H
