
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

struct Hsm_event_handler_pair const SROOT_event_handlers[] = {
    { HSM_EVENT_STATE_INIT,  &SROOT_init   },
    { E0,                    &SROOT_E0     },
    { E1,                    &SROOT_E1     }
};

struct Hsm_event_handler_pair const S0_event_handlers[] = {
    { HSM_EVENT_STATE_INIT,  &S0_init   },
    { HSM_EVENT_STATE_ENTRY, &S0_entry  },
    { HSM_EVENT_STATE_EXIT,  &S0_exit   }, 
    { E0,                    &S0_E0     },
    { E4,                    &S0_E4     }
};

struct Hsm_event_handler_pair const S1_event_handlers[] = {
    { HSM_EVENT_STATE_INIT,  &S1_init   },
    { HSM_EVENT_STATE_ENTRY, &S1_entry  },
    { HSM_EVENT_STATE_EXIT,  &S1_exit   }, 
    { E3,                    &S1_E3     },
    { E4,                    &S1_E4     }
};

struct Hsm_event_handler_pair const S2_event_handlers[] = {
    { HSM_EVENT_STATE_ENTRY, &S2_entry  },
    { HSM_EVENT_STATE_EXIT,  &S2_exit   }, 
    { E0,                    &S2_E0     },
    { E2,                    &S2_E2     },
    { E3,                    &S2_E3     }
};

struct Hsm_event_handler_pair const S3_event_handlers[] = {
    { HSM_EVENT_STATE_ENTRY, &S3_entry  },
    { HSM_EVENT_STATE_EXIT,  &S3_exit   }, 
    { E2,                    &S3_E2     },
    { E3,                    &S3_E3     },
    { E4,                    &S3_E4     }
};

struct Hsm_event_handler_pair const S4_event_handlers[] = {
    { HSM_EVENT_STATE_ENTRY, &S4_entry  },
    { HSM_EVENT_STATE_EXIT,  &S4_exit   }, 
    { E1,                    &S4_E1     },
    { E2,                    &S4_E2     },
    { E4,                    &S4_E4     }
};

struct Hsm_event_handler_pair const S5_event_handlers[] = {
    { HSM_EVENT_STATE_ENTRY, &S5_entry  },
    { HSM_EVENT_STATE_EXIT,  &S5_exit   }, 
    { E1,                    &S5_E1     },
    { E3,                    &S5_E3     },
    { E4,                    &S5_E4     }
};

int main()
{
    Hsm_handle hsm = hsm_create(NUM_STATES, NUM_EVENTS);
    hsm_state_add_child(hsm, HSM_STATE_ROOT, S0);
    hsm_state_add_child(hsm, S0, S2);
    hsm_state_add_child(hsm, S0, S3);

    hsm_state_add_child(hsm, HSM_STATE_ROOT, S1);
    hsm_state_add_child(hsm, S1, S4);
    hsm_state_add_child(hsm, S1, S5);


    hsm_state_set_event_handler_table(hsm, HSM_STATE_ROOT, SROOT_event_handlers, HANDLER_TABLE_SIZE(SROOT_event_handlers));
    hsm_state_set_event_handler_table(hsm, S0, S0_event_handlers, HANDLER_TABLE_SIZE(S0_event_handlers));
    hsm_state_set_event_handler_table(hsm, S1, S1_event_handlers, HANDLER_TABLE_SIZE(S1_event_handlers));
    hsm_state_set_event_handler_table(hsm, S2, S2_event_handlers, HANDLER_TABLE_SIZE(S2_event_handlers));
    hsm_state_set_event_handler_table(hsm, S3, S3_event_handlers, HANDLER_TABLE_SIZE(S3_event_handlers));
    hsm_state_set_event_handler_table(hsm, S4, S4_event_handlers, HANDLER_TABLE_SIZE(S4_event_handlers));
    hsm_state_set_event_handler_table(hsm, S5, S5_event_handlers, HANDLER_TABLE_SIZE(S5_event_handlers));

    hsm_finalise_structure(hsm);

    hsm_dispatch(hsm, E4, NULL);
    
    

    return 0;
}