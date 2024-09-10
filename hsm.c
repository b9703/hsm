

#include "hsm.h"
#include <assert.h>

#ifdef HSM_ASSERTIONS
#define HSM_ASSERT(cond)    assert(cond)
#else
#define HSM_ASSERT(cond)    
#endif


static struct Hsm_event const EVENT_STATE_ENTRY = { .type = HSM_EVENT_TYPE_STATE_ENTRY };
static struct Hsm_event const EVENT_STATE_EXIT  = { .type = HSM_EVENT_TYPE_STATE_EXIT  };
static struct Hsm_event const EVENT_STATE_INIT  = { .type = HSM_EVENT_TYPE_STATE_INIT };

struct Path
{
    struct Hsm_state * nodes[MAX_HSM_DEPTH];
    uint32_t num_nodes;
};

static void path_init(struct Path * p_path)
{
    for (uint32_t i = 0; i < MAX_HSM_DEPTH; i++) { p_path->nodes[i] = HSM_STATE_NULL; }
    p_path->num_nodes = 0;
}

static void path_add_node(struct Path * p_path, struct Hsm_state * p_node)
{
    assert(p_path->num_nodes != MAX_HSM_DEPTH);
    p_path->nodes[p_path->num_nodes++] = p_node;
}

static bool path_check_contains_node(struct Path const * p_path,
                                     struct Hsm_state const * p_node,
                                     uint32_t * p_index)
{
    for (uint32_t i = 0; i < p_path->num_nodes; i++)
    {
        if (p_path->nodes[i] == p_node) 
        {
            if (p_index != NULL)
            {
                *p_index = i;
            }
            return true;
        }
    }

    return false;
}

static void get_path_to_root(struct Path * p_path, struct Hsm_state * p_start)
{
    path_init(p_path);
    struct Hsm_state * p_curr = p_start;
    do { path_add_node(p_path, p_curr); } while ((p_curr = p_curr->p_super) != HSM_STATE_NULL);
}

static bool get_path_intersection(struct Path const * p_path_a,
                                  struct Path const * p_path_b,
                                  uint32_t * p_path_a_index,
                                  uint32_t * p_path_b_index)
{
    for (uint32_t i = 0; i < p_path_b->num_nodes; i++)
    {
        uint32_t path_a_index;
        if (path_check_contains_node(p_path_a, p_path_b->nodes[i], &path_a_index))
        {
            *p_path_a_index = path_a_index;
            *p_path_b_index = i;
            return true;
        }
    }

    return false;
}

static uint32_t state_get_num_substates(struct Hsm_state const * p_state)
{
    uint32_t n = 0;
    while (p_state->p_substates[n++] != HSM_STATE_NULL);
    return n;
}

static bool state_is_at_bottom_of_hierarchy(struct Hsm_state const * p_state)
{
    return (state_get_num_substates(p_state) == 0);
}

static bool check_entry_handling_is_valid(struct Hsm_handling const * p_handling)
{
    // Entry handler cannot result in a transition.
    return (p_handling->ret != HSM_HANDLING_RET_HANDLED_WITH_TRANSITION); 
}

static bool check_exit_handling_is_valid(struct Hsm_handling const * p_handling)
{
    // Exit handler cannot result in a transition.
    return (p_handling->ret != HSM_HANDLING_RET_HANDLED_WITH_TRANSITION); 
}

static void do_state_entry_actions(struct Hsm * p_hsm, struct Hsm_state const * p_state)
{
    struct Hsm_handling h = p_state->handle_event(&EVENT_STATE_ENTRY, p_hsm->p_context);
    HSM_ASSERT(&h);
}

static void do_state_exit_actions(struct Hsm * p_hsm, struct Hsm_state const * p_state)
{
    struct Hsm_handling h = p_state->handle_event(&EVENT_STATE_EXIT, p_hsm->p_context);
    HSM_ASSERT(&h);
}

static struct Hsm_state * do_state_init_handling(struct Hsm * p_hsm, struct Hsm_state const * p_state)
{
    struct Hsm_state const * p_curr = p_state;
    struct Hsm_handling handling;
    while (true)
    {
        handling = p_curr->handle_event(&EVENT_STATE_INIT, p_hsm->p_context);
        if (handling.ret == HSM_HANDLING_RET_UNHANDLED)
        {
            break;
        }
        else if (handling.ret == HSM_HANDLING_RET_HANDLED_WITH_TRANSITION)
        {
            HSM_ASSERT((p_curr == handling.p_target->p_super));
            p_curr = handling.transition.p_target;
        }
        else
        {
            HSM_ASSERT(false);
        }
    }

    HSM_ASSERT(state_is_at_bottom_of_hierarchy(p_curr));
    return p_curr; 
}

/**************************************************************************************************
 * Calls all of the entry, exit, and init actions that must take place in order to transition 
 * from the current state of the HSM to the target state. Note: the handler state is the state
 * who's handler actually triggered the transition. It must be the current state, or an ancestor
 * of it. 
 *************************************************************************************************/
static void transition_state(struct Hsm * p_hsm, 
                                 struct Hsm_transition const * p_transition,
                                 struct Hsm_state const * p_handler_state)
{
    // Do exit action up to (but not including) the state that triggered the transition.
    struct Hsm_state const * p_curr = p_hsm->p_curr_state;
    while (p_curr != p_handler_state)
    {
        do_state_exit_actions(p_hsm, p_curr);
        p_curr = p_curr->p_super;
    }     

    struct Path curr_path_to_root;
    struct Path target_path_to_root;
    get_path_to_root(&curr_path_to_root, p_curr);
    get_path_to_root(&target_path_to_root, p_transition->p_target);
    
    /* The least-common-ancestor (LCA) of the curr and target states is the point where the paths 
     * up to the root state insersect.
     */
    uint32_t lca_index_in_curr_path;
    uint32_t lca_index_in_target_path; 
    get_path_intersection(&curr_path_to_root, &target_path_to_root,
                          &lca_index_in_curr_path, &lca_index_in_target_path);


    /* At this point, if curr is the LCA then this means the target is either curr itself, or a
     * substate of curr. In either case, if the transition is an external one then we need to
     * first exit, then re-enter curr.
     */   
    struct Hsm_state const * p_lca = curr_path_to_root.nodes[lca_index_in_curr_path];
    if (p_curr == p_lca)
    {
        if (p_transition->type == HSM_TRANSITION_TYPE_EXTERNAL)
        {
            do_state_exit_actions(p_hsm, p_curr);
            do_state_entry_actions(p_hsm, p_curr);
        }
    }
    else // LCA must be higher up the state hierarchy so we need to go up... 
    {
        while (p_curr != p_lca)
        {
            do_state_exit_actions(p_hsm, p_curr);
            p_curr = p_curr->p_super;
        }

    }
    
    /* Now we need to go down the hierarchy, following the target's path to root in reverse
     * starting from the state below the LCA, until the target is reached.
     */
    for (int32_t i = (lca_index_in_target_path-1); i >= 0; i--)
    {
        p_curr = target_path_to_root.nodes[i];
        do_state_entry_actions(p_hsm, p_curr);
    }    

    do_state_init_handling(p_hsm, p_curr);
}

static void handle_event(struct Hsm * p_hsm,
                         struct Hsm_event const * p_event,
                         struct Hsm_handling * p_handling,
                         struct Hsm_state ** p_handler_state)
{
    // Go up the start hierarchy until event is handled. 
    struct Hsm_handling handling;
    struct Hsm_state const * p_curr = p_hsm->p_curr_state;
    while (p_curr != HSM_STATE_NULL)
    {
        handling = p_curr->handle_event(p_event, p_hsm->p_context);
        if (handling.ret != HSM_HANDLING_RET_UNHANDLED)
        {
            break;
        }
        p_curr = p_curr->p_super;
    }
    
    *p_handling = handling;
    *p_handler_state = p_curr;
}

/**************************************************************************************************
 * HSM API function implementations
 *************************************************************************************************/

void hsm_start(struct Hsm * p_hsm)
{
    do_state_init_handling(p_hsm, p_hsm->p_root_state);
}

enum Hsm_ret hsm_handle_event(struct Hsm * p_hsm, struct Hsm_event const * p_event)
{
    assert(p_hsm != NULL);
    assert(p_event != NULL);
    assert(p_hsm->p_curr_state != HSM_STATE_NULL);

    struct Hsm_handling handling;
    struct Hsm_state * p_handler_state;
    handle_event(p_hsm, p_event, &handling, &p_handler_state);
    if (handling.ret == HSM_HANDLING_RET_HANDLED_WITH_TRANSITION)
    {
        transition_state(p_hsm, &handling.transition, p_handler_state); 
    }

    return (p_hsm->p_curr_state == HSM_STATE_FINAL) ? HSM_RET_REACHED_FINAL_STATE
                                                    : HSM_RET_UNDERWAY;
}

void hsm_abort(struct Hsm * p_hsm)
{

}

void hsm_set_meta_settings(struct Hsm_meta_settings const * p_settings)
{

}