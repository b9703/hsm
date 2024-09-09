

#ifndef HSM_H 
#define HSM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 *
 */
#ifndef HSM_MAX_NUM_SUBSTATES   
#define HSM_MAX_NUM_SUBSTATES   16
#endif

/*
 *
 */
#ifndef MAX_HSM_DEPTH   
#define MAX_HSM_DEPTH   8
#endif

typedef int32_t Hsm_event_type;

// TODO: consider representing the final state differently.
#define HSM_STATE_NULL    (struct Hsm_state *)NULL
#define HSM_STATE_FINAL   (struct Hsm_state *)1;

/* Event type 
 *
 */
#define HSM_EVENT_TYPE_STATE_ENTRY  ((Hsm_event_type)-3)
#define HSM_EVENT_TYPE_STATE_EXIT   ((Hsm_event_type)-2)
#define HSM_EVENT_TYPE_STATE_INIT   ((Hsm_event_type)-1)
#define HSM_USER_EVENT_TYPES_START  ((Hsm_event_type)0)

typedef void (*Hsm_logging_callback)(char const * log_string);
typedef void (*Hsm_assert_callback)();

struct Hsm_meta_settings
{
    Hsm_logging_callback logging_callback;
    Hsm_assert_callback assert_callback;
};

enum Hsm_ret
{
    HSM_RET_UNDERWAY,
    HSM_RET_FINISHED,
};

enum Hsm_handling_ret
{
    // Event was unhandled (so it will be passed to its super-state).
    HSM_HANDLING_RET_UNHANDLED,

    // Event was handled but no transition resulted (HSM stays in the same state).
    HSM_HANDLING_RET_HANDLED_NO_TRANSITION,

    // Event was handled and a transition was requested.
    HSM_HANDLING_RET_HANDLED_WITH_TRANSITION
};

enum Hsm_transition_type
{
    HSM_TRANSITION_TYPE_DEFAULT,
    HSM_TRANSITION_TYPE_EXTERNAL,
    HSM_TRANSITION_TYPE_LOCAL,
    HSM_TRANSITION_TYPE_NONE,
}

struct Hsm_handling
{
    /* If ret == HSM_HANDLING_RET_HANDLED_WITH_TRANSITION then the p_target member is used to 
     * specify the target state for the tranisition, otherwise p_target is ignored.
     * 
     */
    enum Hsm_handling_ret ret;
    struct Hsm_state * p_target;
    enum Hsm_transition_type transition_type;
};

/* Macros for using in state event handlers. For example...
 * struct Hsm_handling handling = HSM_EVENT_HANDLED();
 * switch (p_event->type)
 * {
 *     case MY_EVENT_TYPE_1:
 *         <event handling code that does not produce a state transition>
 *         break;
 *     
 *     case MY_EVENT_TYPE_2:
 *         handling = HSM_TRANSITION(&some_state);
 *         break;
 * 
 *     default:
 *         handling = HSM_EVENT_UNHANDLED()
 * }
 *
 */
#define HSM_EVENT_UNHANDLED()          { .ret = HSM_HANDLING_RET_UNHANDLED,               .p_target = HSM_STATE_NULL, .transition_type = HSM_TRANSITION_TYPE_NONE     }
#define HSM_EVENT_HANDLED()            { .ret = HSM_HANDLING_RET_HANDLED_NO_TRANSITION,   .p_target = HSM_STATE_NULL, .transition_type = HSM_TRANSITION_TYPE_NONE     }
#define HSM_TRANSITION(state)          { .ret = HSM_HANDLING_RET_HANDLED_WITH_TRANSITION, .p_target = (&state),       .transition_type = HSM_TRANSITION_TYPE_DEFAULT  }
#define HSM_TRANSITION_LOCAL(state)    { .ret = HSM_HANDLING_RET_HANDLED_WITH_TRANSITION, .p_target = (&state),       .transition_type = HSM_TRANSITION_TYPE_LOCAL    }
#define HSM_TRANSITION_EXTERNAL(state) { .ret = HSM_HANDLING_RET_HANDLED_WITH_TRANSITION, .p_target = (&state),       .transition_type = HSM_TRANSITION_TYPE_EXTERNAL }

#define HSM_EVENT_CAST(p_event)     ((struct Hsm_event *)p_event)

/* The base 'class' for HSM events. You can implement custom events by embedding a Hsm_event as the
 * first member of the event. For example...
 * struct My_custom_event
 * {
 *     struct Hsm_event base;
 *     int my_data;
 * };
 * 
 * then when an event pointer is passed to the HSM it should be downcast to a pointer of the 
 * Hsm_event base class e.g. hsm_handle_event(&my_hsm, HSM_EVENT_CAST(&my_custom_event)).
 * The event handlers can then retrieve the data by looking at the event type and casting
 * the event pointer back to the custom events type.
 */
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

typedef struct Hsm_handling (*Hsm_event_handler)(struct Hsm_event const * p_event, void * p_context);


struct Hsm
{
    char const * name;
    struct Hsm_state const * p_root_state;

    /* The handler that gets called when enter the 'final' pseudo state. It is essentially
     * 
     */
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