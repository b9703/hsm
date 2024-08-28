

#include "hsm.h"
#include <assert.h>

static struct Hsm_event const EVENT_STATE_ENTRY = { .type = HSM_EVENT_TYPE_STATE_ENTRY };
static struct Hsm_event const EVENT_STATE_EXIT  = { .type = HSM_EVENT_TYPE_STATE_EXIT  };

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

static bool path_check_contains_node(struct Path const * p_path, struct Hsm_state const * p_node)
{
    for (uint32_t i = 0; i < p_path->num_nodes; i++)
    {
        if (p_path->nodes[i] == p_node) { return true; }
    }

    return false;
}

static struct Hsm_state * path_find_intersection(struct Path const * p_path1, 
                                                 struct Path const * p_path2)
{
    for (uint32_t i = 0; i < p_path1->num_nodes; i++)
    {
        if (path_check_contains_node(p_path2, p_path1->nodes[i]))
        {
            return p_path1->nodes[i];
        }
    }

    return HSM_STATE_NULL;
}

/* The "path to root" means every super state, starting from but no including the start state, up 
* to the root state (inclusive).
*/
static void get_path_to_root(struct Path * p_path, struct Hsm_state * p_start)
{
    path_init(p_path);

    // The root state is the only state with no super-state.
    struct Hsm_state * p_curr = p_start->p_super;
    do { path_add_node(p_path, p_curr); } while ((p_curr = p_curr->p_super) != HSM_STATE_NULL);
}


static struct Hsm_state * get_lca(struct Hsm_state const * p_s1, struct Hsm_state const * p_s2)
{
    struct Path s1_to_root;
    struct Path s2_to_root;
    get_path_to_root(&s1_to_root, p_s1);
    get_path_to_root(&s2_to_root, p_s2);
    return path_find_intersection(&s1_to_root, &s2_to_root);
}

static void transition(struct Hsm * p_hsm, struct Hsm_transition const * p_trans)
{
    struct Hsm_state const * p_lca = get_lca(p_hsm->p_curr_state, p_trans->p_target);
    while (p_hsm->p_curr_state != p_lca)
    {
        p_hsm->p_curr_state->handle_event(p_hsm->p_context, &EVENT_STATE_EXIT);
        p_hsm->p_curr_state = p_hsm->p_curr_state->p_super;
    }

    

    while (p_hsm->p_curr_state != p_trans->p_target)
    {
        p_hsm->p_curr_state->handle_event(p_hsm->p_context, &EVENT_STATE_ENTRY);
        p_hsm->p_curr_state = p_hsm->p_curr_state->p_super;
    }
}

void hsm_start(struct Hsm * p_hsm)
{
    struct Hsm_transition trans = p_hsm->start_handler(p_hsm->p_context, NULL);
    assert(trans.p_target != HSM_STATE_NULL);
    transition(p_hsm, &trans);
}

enum Hsm_ret hsm_handle_event(struct Hsm * p_hsm, struct Hsm_event const * p_event)
{
    assert(p_hsm != NULL);
    assert(p_event != NULL);
    assert(p_hsm->p_curr_state != HSM_STATE_NULL);

    struct Hsm_transition trans = p_hsm->p_curr_state->handle_event(p_hsm->p_context, p_event);
    transition(p_hsm, &trans);

    return (trans.p_target == HSM_STATE_FINAL) ? HSM_RET_FINISHED : HSM_RET_UNDERWAY;
}

void hsm_abort(struct Hsm * p_hsm)
{

}

void hsm_set_meta_settings(struct Hsm_meta_settings const * p_settings)
{

}