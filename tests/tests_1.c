
#include "../hsm.h"
#include "stdio.h"

#define PRINT_FUNC()  printf("called %s\n", __func__);

enum State
{
    S0 = HSM_USER_STATES_START,
    S1,
    S2,
    S3,
    S4,
    NUM_STATES
};

enum Event
{
    E0,
    E1,
    E2,
    E3,
    NUM_EVENTS
};

/**************************************************************************************************
 * SROOT handlers
 *************************************************************************************************/
struct Hsm_trans SROOT_init(void * p_data)
{
    return HSM_TRANS(S0);
}

/**************************************************************************************************
 * S0 handlers
 *************************************************************************************************/
struct Hsm_trans S0_entry(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S0_exit(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S0_E0(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S0);
}

struct Hsm_trans S0_E1(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S1);
}

struct Hsm_trans S0_E2(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S2);
}

struct Hsm_trans S0_E3(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

/**************************************************************************************************
 * S1 handlers
 *************************************************************************************************/
struct Hsm_trans S1_entry(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S1_exit(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}


struct Hsm_trans S1_E2(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S2);
}

struct Hsm_trans S1_E3(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S1); 
}

/**************************************************************************************************
 * S2 handlers
 *************************************************************************************************/
struct Hsm_trans S2_entry(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S2_E1(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S3);
}

struct Hsm_trans S2_E2(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}


/**************************************************************************************************
 * S3 handlers
 *************************************************************************************************/
struct Hsm_trans S3_exit(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}


struct Hsm_trans S3_E1(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S3);
}

struct Hsm_trans S3_E2(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S3_E3(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S0);
}



int main()
{
    Hsm_handle hsm = hsm_create(NUM_STATES, NUM_EVENTS);
    hsm_state_add_child(hsm, HSM_STATE_ROOT, S0);
    hsm_state_add_child(hsm, HSM_STATE_ROOT, S1);
    hsm_state_add_child(hsm, HSM_STATE_ROOT, S2);
    hsm_state_add_child(hsm, HSM_STATE_ROOT, S3);

    hsm_state_set_event_handler(hsm, HSM_STATE_ROOT, HSM_EVENT_STATE_INIT, &SROOT_init);

    hsm_state_set_event_handler(hsm, S0, HSM_EVENT_STATE_ENTRY, &S0_entry);
    hsm_state_set_event_handler(hsm, S0, HSM_EVENT_STATE_EXIT,  &S0_exit);
    hsm_state_set_event_handler(hsm, S0, E0, &S0_E0);
    hsm_state_set_event_handler(hsm, S0, E1, &S0_E1);
    hsm_state_set_event_handler(hsm, S0, E2, &S0_E2);
    hsm_state_set_event_handler(hsm, S0, E3, &S0_E3);

    hsm_state_set_event_handler(hsm, S1, HSM_EVENT_STATE_ENTRY, &S1_entry);
    hsm_state_set_event_handler(hsm, S1, HSM_EVENT_STATE_EXIT,  &S1_exit);
    hsm_state_set_event_handler(hsm, S1, E2, &S1_E2);
    hsm_state_set_event_handler(hsm, S1, E3, &S1_E3);

    hsm_state_set_event_handler(hsm, S2, HSM_EVENT_STATE_ENTRY, &S2_entry);
    hsm_state_set_event_handler(hsm, S2, E1, &S2_E1);
    hsm_state_set_event_handler(hsm, S2, E2, &S2_E2);

    hsm_state_set_event_handler(hsm, S3, HSM_EVENT_STATE_EXIT,  &S3_exit);
    hsm_state_set_event_handler(hsm, S3, E1, &S3_E1);
    hsm_state_set_event_handler(hsm, S3, E2, &S3_E2);
    hsm_state_set_event_handler(hsm, S3, E3, &S3_E3);

    hsm_finalise_structure(hsm);

    hsm_dispatch(hsm, E3, NULL);
    hsm_dispatch(hsm, E0, NULL);
    hsm_dispatch(hsm, E1, NULL);
    hsm_dispatch(hsm, E3, NULL);
    hsm_dispatch(hsm, E0, NULL);
    hsm_dispatch(hsm, E1, NULL);
    hsm_dispatch(hsm, E2, NULL);
    hsm_dispatch(hsm, E2, NULL);
    hsm_dispatch(hsm, E1, NULL);
    hsm_dispatch(hsm, E2, NULL);
    hsm_dispatch(hsm, E1, NULL);
    hsm_dispatch(hsm, E3, NULL);
    hsm_dispatch(hsm, E2, NULL);
    

    return 0;
}