

#ifndef HSM_H
#define HSM_H

enum 
{
    HSM_NO_STATE_TRANSITION = -2,
    HSM_NULL_STATE          = -1,
    HSM_ROOT_STATE          = 0,
    HSM_USER_STATES_START   = 1
};

enum
{
    EVENT_HSM_STATE_ENTRY = -3,
    EVENT_HSM_STATE_EXIT  = -2,
    EVENT_HSM_STATE_INIT  = -1,
    HSM_USER_EVENTS_START = 0
};

struct Hsm;
typedef struct Hsm * Hsm_handle;

typedef int (*Event_handler)(void * p_data);

Hsm_handle hsm_create(int num_states, int num_events);

void hsm_add_state_event_handler(Hsm_handle hsm, int state, int event, Event_handler handler);

void hsm_add_state_child(Hsm_handle hsm, int state, int child);

void hsm_finalise_structure(Hsm_handle hsm);

void hsm_dispatch(Hsm_handle hsm, int event, void * p_data);


#endif // HSM_H
