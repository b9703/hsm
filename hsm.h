

#ifndef HSM_H 
#define HSM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef HSM_MAX_NUM_IMMEDIATE_SUBSTATES_PER_STATE   
#define HSM_MAX_NUM_IMMEDIATE_SUBSTATES_PER_STATE   16
#endif

#ifndef HSM_MAX_DEPTH   
#define HSM_MAX_DEPTH   8
#endif

#ifndef HSM_MAX_NAME_LENGTH      
#define HSM_MAX_NAME_LENGTH   32
#endif

typedef int32_t Hsm_event_type;

// TODO: consider representing the final state differently.
#define HSM_STATE_NULL    (struct Hsm_state *)NULL
#define HSM_STATE_FINAL   (struct Hsm_state *)1

// Special event types
#define HSM_EVENT_TYPE_STATE_ENTRY  ((Hsm_event_type)-3)
#define HSM_EVENT_TYPE_STATE_EXIT   ((Hsm_event_type)-2)
#define HSM_EVENT_TYPE_STATE_INIT   ((Hsm_event_type)-1)

// Where user-defined event types must start from.
#define HSM_USER_EVENT_TYPES_START  ((Hsm_event_type)0)

/*
 *
 */
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
    HSM_RET_REACHED_FINAL_STATE,
};

enum Hsm_handling_ret
{
    HSM_HANDLING_RET_UNHANDLED,
    HSM_HANDLING_RET_HANDLED_NO_TRANSITION,
    HSM_HANDLING_RET_HANDLED_WITH_TRANSITION
};

enum Hsm_transition_type
{
    HSM_TRANSITION_TYPE_DEFAULT,
    HSM_TRANSITION_TYPE_EXTERNAL,
    HSM_TRANSITION_TYPE_LOCAL,
    HSM_TRANSITION_TYPE_NONE,
};

struct Hsm_transition
{
    enum Hsm_transition_type type;
    struct Hsm_type const * p_target;
};

struct Hsm_handling
{
    // transition is only valid if ret indicates that a transition should occur.
    enum Hsm_handling_ret ret;
    struct Hsm_transition transition;
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
 *     case MY_EVENT_TYPE_3:
 *         handling = HSM_TRANSITION_EXTERNAL(&some_super_state);
 * 
 *     default:
 *         handling = HSM_EVENT_UNHANDLED();
 * }
 *
 */
#define HSM_EVENT_UNHANDLED()          { .ret = HSM_HANDLING_RET_UNHANDLED,               .p_target = HSM_STATE_NULL, .transition_type = HSM_TRANSITION_TYPE_NONE     }
#define HSM_EVENT_HANDLED()            { .ret = HSM_HANDLING_RET_HANDLED_NO_TRANSITION,   .p_target = HSM_STATE_NULL, .transition_type = HSM_TRANSITION_TYPE_NONE     }
#define HSM_TRANSITION(state)          { .ret = HSM_HANDLING_RET_HANDLED_WITH_TRANSITION, .p_target = (&state),       .transition_type = HSM_TRANSITION_TYPE_DEFAULT  }
#define HSM_TRANSITION_LOCAL(state)    { .ret = HSM_HANDLING_RET_HANDLED_WITH_TRANSITION, .p_target = (&state),       .transition_type = HSM_TRANSITION_TYPE_LOCAL    }
#define HSM_TRANSITION_EXTERNAL(state) { .ret = HSM_HANDLING_RET_HANDLED_WITH_TRANSITION, .p_target = (&state),       .transition_type = HSM_TRANSITION_TYPE_EXTERNAL }


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
 * the event pointer back to the custom events type. For example...
 * switch (p_event->type)
 * {
 *     case EVENT_BUTTON_PRESS:
 *     {
 *         struct Event_button_press * p_ev = (struct Event_button_press *)p_event;
 *         if (p_ev->button_id) { ... }
 *     }
 * }
 */
struct Hsm_event
{
    Hsm_event_type type;  
};

// Used to cast an Hsm_event derived class pointer back to the base class.
#define HSM_EVENT_CAST(p_event)     ((struct Hsm_event *)p_event)

/*
 *
 */
struct Hsm_state
{
    // Only important for logging, if enabled.
    char name[HSM_MAX_NAME_LENGTH];
     
    Hsm_event_handler const handle_event;    

    struct Hsm_state const * p_super;

    // Null-terminated array of substates.
    struct Hsm_state const * p_substates[HSM_MAX_NUM_IMMEDIATE_SUBSTATES_PER_STATE+1];
};

// State event handler prototype.
typedef struct Hsm_handling (*Hsm_event_handler)(struct Hsm_event const * p_event, void * p_context);

/*
 *
 *
 */
struct Hsm
{
    // Only important for logging, if enabled.
    char name[HSM_MAX_NAME_LENGTH];

    /* All HSMs must have a root state that contains all other states. 
     * 
     */
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
 * Starts the HSM, causing it to perform its initial transition i.e. pass an init state event to 
 * the root state.
 *************************************************************************************************/
void hsm_start(struct Hsm * p_hsm);

/**************************************************************************************************
 * Gives an event to the HSM for handling. The HSM must have been started. If the HSM has already
 * reached its final state then no event handling will be done (and a corresponding return code
 * will indicate this).
 *************************************************************************************************/
enum Hsm_ret hsm_handle_event(struct Hsm * p_hsm, struct Hsm_event const * p_event);

/**************************************************************************************************
 * Aborts the state machine operation immediately.
 *************************************************************************************************/
void hsm_abort(struct Hsm * p_hsm);

/**************************************************************************************************
 *
 *************************************************************************************************/
void hsm_set_meta_settings(struct Hsm_meta_settings const * p_settings);

#endif // HSM_H