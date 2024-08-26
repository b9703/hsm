

#ifndef HSM_H 
#define HSM_H

#include <stdint.h>
#include <stdbool.h>

#ifndef HSM_MAX_NUM_SUBSTATES   
#define HSM_MAX_NUM_SUBSTATES   16
#endif

#define HSM_STATE_NULL    (struct Hsm_state *)NULL


typedef int32_t Hsm_event_type;

enum Hsm_ret
{
    HSM_RET_UNDERWAY,
    HSM_RET_FINISHED,
};

enum Hsm_transition_type
{
    HSM_TRANSITION_TYPE_DEFAULT,
    HSM_TRANSITION_TYPE_LOCAL,
    HSM_TRANSITION_TYPE_EXTERNAL,
};

struct Hsm_transition
{
    struct Hsm_state * p_target;
    enum Hsm_transition_type type;
};

#define HSM_TRANS_DEFAULT()     { .p_target = HSM_STATE_NULL, .type = HSM_TRANSITION_TYPE_DEFAULT }


struct Hsm_meta_settings
{
    Hsm_logging_callback logging_callback;
    Hsm_assert_callback assert_callback;
};

enum Hsm_handling_status 
{
};


struct Hsm_event
{
   Hsm_event_type type;  
};

struct Hsm_state
{
    char const * name;
    Hsm_event_handler const handle_event;    
    struct Hsm_state const * p_super;

    // Null-terminated array of substates.
    struct Hsm_state const * p_substates[HSM_MAX_NUM_SUBSTATES+1];
};

typedef struct Hsm_state * (*Hsm_event_handler)(void * p_context, struct Hsm_event const * p_event);

typedef void (*Hsm_logging_callback)(char const * log_string);
typedef void (*Hsm_assert_callback)();

struct Hsm
{
    char const * name;
    struct Hsm_state const * p_root_state;
    Hsm_event_handler const start_handler;
    Hsm_event_handler const final_handler;
    void * p_context;    

    // Internal member variables (not to be modified by application code).    
    struct Hsm_state * p_curr_state;
    bool is_finished;
};

/**************************************************************************************************
 *
 *************************************************************************************************/
void hsm_start(struct Hsm * p_hsm);

/**************************************************************************************************
 *
 *************************************************************************************************/
enum Hsm_ret hsm_handle_event(struct Hsm * p_hsm, struct Hsm_event const * p_event);

/**************************************************************************************************
 *
 *************************************************************************************************/
void hsm_abort(struct Hsm * p_hsm);

/**************************************************************************************************
 *
 *************************************************************************************************/
void hsm_set_meta_settings(struct Hsm_meta_settings const * p_settings);

#endif // HSM_H