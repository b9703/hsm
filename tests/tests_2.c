
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
    S5,
    NUM_STATES
};

enum Event
{
    E0,
    E1,
    E2,
    E3,
    E4,
    NUM_EVENTS
};

/**************************************************************************************************
 * SROOT handlers
 *************************************************************************************************/
struct Hsm_trans SROOT_init(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S0);
}

struct Hsm_trans SROOT_E1(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans SROOT_E0(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_LOCAL(S4);
}

/**************************************************************************************************
 * S0 handlers
 *************************************************************************************************/
struct Hsm_trans S0_init(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S2);
}

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

struct Hsm_trans S0_E4(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S1);
}

/**************************************************************************************************
 * S1 handlers
 *************************************************************************************************/
struct Hsm_trans S1_init(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S5);
}

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

struct Hsm_trans S1_E3(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_LOCAL(S5);
}

struct Hsm_trans S1_E4(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S0);
}

/**************************************************************************************************
 * S2 handlers
 *************************************************************************************************/
struct Hsm_trans S2_entry(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S2_exit(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S2_E0(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S2_E2(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S3);
}

struct Hsm_trans S2_E3(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_EXTERNAL(S0);
}

/**************************************************************************************************
 * S3 handlers
 *************************************************************************************************/
struct Hsm_trans S3_entry(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S3_exit(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}


struct Hsm_trans S3_E2(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S2);
}

struct Hsm_trans S3_E3(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S4);
}

struct Hsm_trans S3_E4(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_LOCAL(S0);
}

/**************************************************************************************************
 * S4 handlers
 *************************************************************************************************/
struct Hsm_trans S4_entry(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S4_exit(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S4_E1(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S1);
}

struct Hsm_trans S4_E2(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S5);
}

struct Hsm_trans S4_E4(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S4);
}

/**************************************************************************************************
 * S5 handlers
 *************************************************************************************************/
struct Hsm_trans S5_entry(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S5_exit(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}

struct Hsm_trans S5_E1(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS(S4);
}

struct Hsm_trans S5_E3(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_LOCAL(S5);
}

struct Hsm_trans S5_E4(void * p_data)
{
    PRINT_FUNC();
    return HSM_TRANS_NONE();
}


int main()
{
    Hsm_handle hsm = hsm_create(NUM_STATES, NUM_EVENTS);
    hsm_state_add_child(hsm, HSM_STATE_ROOT, S0);
    hsm_state_add_child(hsm, S0, S2);
    hsm_state_add_child(hsm, S0, S3);

    hsm_state_add_child(hsm, HSM_STATE_ROOT, S1);
    hsm_state_add_child(hsm, S1, S4);
    hsm_state_add_child(hsm, S1, S5);

    hsm_state_set_event_handler(hsm, HSM_STATE_ROOT, HSM_EVENT_STATE_INIT, &SROOT_init);
    hsm_state_set_event_handler(hsm, HSM_STATE_ROOT, E0, &SROOT_E0);
    hsm_state_set_event_handler(hsm, HSM_STATE_ROOT, E1, &SROOT_E1);

    hsm_state_set_event_handler(hsm, S0, HSM_EVENT_STATE_INIT, &S0_init);
    hsm_state_set_event_handler(hsm, S0, HSM_EVENT_STATE_ENTRY, &S0_entry);
    hsm_state_set_event_handler(hsm, S0, HSM_EVENT_STATE_EXIT,  &S0_exit);
    hsm_state_set_event_handler(hsm, S0, E0, &S0_E0);
    hsm_state_set_event_handler(hsm, S0, E4, &S0_E4);

    hsm_state_set_event_handler(hsm, S1, HSM_EVENT_STATE_INIT, &S1_init);
    hsm_state_set_event_handler(hsm, S1, HSM_EVENT_STATE_ENTRY, &S1_entry);
    hsm_state_set_event_handler(hsm, S1, HSM_EVENT_STATE_EXIT,  &S1_exit);
    hsm_state_set_event_handler(hsm, S1, E3, &S1_E3);
    hsm_state_set_event_handler(hsm, S1, E4, &S1_E4);

    hsm_state_set_event_handler(hsm, S2, HSM_EVENT_STATE_ENTRY, &S2_entry);
    hsm_state_set_event_handler(hsm, S2, HSM_EVENT_STATE_EXIT, &S2_exit);
    hsm_state_set_event_handler(hsm, S2, E0, &S2_E0);
    hsm_state_set_event_handler(hsm, S2, E2, &S2_E2);
    hsm_state_set_event_handler(hsm, S2, E3, &S2_E3);

    hsm_state_set_event_handler(hsm, S3, HSM_EVENT_STATE_ENTRY, &S3_entry);
    hsm_state_set_event_handler(hsm, S3, HSM_EVENT_STATE_EXIT,  &S3_exit);
    hsm_state_set_event_handler(hsm, S3, E2, &S3_E2);
    hsm_state_set_event_handler(hsm, S3, E3, &S3_E3);
    hsm_state_set_event_handler(hsm, S3, E4, &S3_E4);

    hsm_state_set_event_handler(hsm, S4, HSM_EVENT_STATE_ENTRY, &S4_entry);
    hsm_state_set_event_handler(hsm, S4, HSM_EVENT_STATE_EXIT,  &S4_exit);
    hsm_state_set_event_handler(hsm, S4, E1, &S4_E1);
    hsm_state_set_event_handler(hsm, S4, E2, &S4_E2);
    hsm_state_set_event_handler(hsm, S4, E4, &S4_E4);


    hsm_state_set_event_handler(hsm, S5, HSM_EVENT_STATE_ENTRY, &S5_entry);
    hsm_state_set_event_handler(hsm, S5, HSM_EVENT_STATE_EXIT,  &S5_exit);
    hsm_state_set_event_handler(hsm, S5, E1, &S5_E1);
    hsm_state_set_event_handler(hsm, S5, E3, &S5_E3);
    hsm_state_set_event_handler(hsm, S5, E4, &S5_E4);

    hsm_finalise_structure(hsm);

    hsm_dispatch(hsm, E4, NULL);
    
    

    return 0;
}