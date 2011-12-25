/*
 *   Copyright 2011 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <zcl/tree.h>

#define __BLACK      (0)
#define __RED        (1)

struct rb_data {
    z_tree_node_t *pa[Z_TREE_MAX_HEIGHT];/* Nodes on stack. */
    unsigned char da[Z_TREE_MAX_HEIGHT]; /* Directions moved from stack nodes */
    int k;                               /* Stack height. */
};

static z_tree_node_t **__do_insert_lookup (const z_tree_info_t *tree,
                                           struct rb_data *v,
                                           z_tree_node_t **root,
                                           void *data)
{
    z_tree_node_t *p;

    v->pa[0] = (z_tree_node_t *)root;
    v->da[0] = 0;
    v->k = 1;
    for (p = *root; p != NULL; p = p->child[v->da[v->k - 1]]) {
        int cmp = tree->key_compare(tree->user_data, p->data, data);
        if (cmp == 0) {
            if (p->data != data && tree->data_free != NULL)
                tree->data_free(tree->user_data, p->data);

            p->data = data;
            return(NULL);
        }

        v->pa[v->k] = p;
        v->da[v->k++] = cmp < 0;
    }

    return(&(v->pa[v->k-1]->child[v->da[v->k-1]]));
}

static int __do_insert (const z_tree_info_t *tree,
                        struct rb_data *v,
                        z_tree_node_t **root,
                        z_tree_node_t *n)
{
    z_tree_node_t *y;

    n->child[0] = n->child[1] = NULL;
    n->balance = __RED;

    while (v->k >= 3 && v->pa[v->k - 1]->balance == __RED) {
        if (v->da[v->k - 2] == 0) {
            y = v->pa[v->k - 2]->child[1];

            if (y != NULL && y->balance == __RED) {
                v->pa[v->k - 1]->balance = y->balance = __BLACK;
                v->pa[v->k - 2]->balance = __RED;
                v->k -= 2;
            } else {
                z_tree_node_t *x;

                if (v->da[v->k - 1] == 0) {
                    y = v->pa[v->k - 1];
                } else {
                    x = v->pa[v->k - 1];
                    y = x->child[1];
                    x->child[1] = y->child[0];
                    y->child[0] = x;
                    v->pa[v->k - 2]->child[0] = y;
                }

                x = v->pa[v->k - 2];
                x->balance = __RED;
                y->balance = __BLACK;

                x->child[0] = y->child[1];
                y->child[1] = x;
                v->pa[v->k - 3]->child[v->da[v->k - 3]] = y;
                break;
            }
        } else {
            y = v->pa[v->k - 2]->child[0];

            if (y != NULL && y->balance == __RED) {
                v->pa[v->k - 1]->balance = y->balance = __BLACK;
                v->pa[v->k - 2]->balance = __RED;
                v->k -= 2;
            } else {
                z_tree_node_t *x;

                if (v->da[v->k - 1] == 1) {
                    y = v->pa[v->k - 1];
                } else {
                    x = v->pa[v->k - 1];
                    y = x->child[0];
                    x->child[0] = y->child[1];
                    y->child[1] = x;
                    v->pa[v->k - 2]->child[1] = y;
                }

                x = v->pa[v->k - 2];
                x->balance = __RED;
                y->balance = __BLACK;

                x->child[1] = y->child[0];
                y->child[0] = x;
                v->pa[v->k - 3]->child[v->da[v->k - 3]] = y;
                break;
            }
        }
    }
    (*root)->balance = __BLACK;

    return(0);
}

static void __do_remove (const z_tree_info_t *tree,
                         struct rb_data *v,
                         z_tree_node_t *p)
{
    if (p->child[1] == NULL) {
        v->pa[v->k - 1]->child[v->da[v->k - 1]] = p->child[0];
    } else {
        z_tree_node_t *r = p->child[1];
        int t;

        if (r->child[0] == NULL) {
            r->child[0] = p->child[0];
            t = r->balance;
            r->balance = p->balance;
            p->balance = t;
            v->pa[v->k - 1]->child[v->da[v->k - 1]] = r;
            v->da[v->k] = 1;
            v->pa[v->k++] = r;
        } else {
            z_tree_node_t *s;
            int j = v->k++;

            for (;;) {
                v->da[v->k] = 0;
                v->pa[v->k++] = r;
                s = r->child[0];
                if (s->child[0] == NULL)
                    break;

                r = s;
            }

            v->da[j] = 1;
            v->pa[j] = s;
            v->pa[j - 1]->child[v->da[j - 1]] = s;

            s->child[0] = p->child[0];
            r->child[0] = s->child[1];
            s->child[1] = p->child[1];

            t = s->balance;
            s->balance = p->balance;
            p->balance = t;
        }
    }

    if (p->balance == __BLACK) {
        z_tree_node_t *w;
        z_tree_node_t *x;

        for (;;) {
            x = v->pa[v->k - 1]->child[v->da[v->k - 1]];
            if (x != NULL && x->balance == __RED) {
                x->balance = __BLACK;
                break;
            }

            if (v->k < 2)
                break;

            if (v->da[v->k - 1] == 0) {
                w = v->pa[v->k - 1]->child[1];

                if (w->balance == __RED) {
                    w->balance = __BLACK;
                    v->pa[v->k - 1]->balance = __RED;

                    v->pa[v->k - 1]->child[1] = w->child[0];
                    w->child[0] = v->pa[v->k - 1];
                    v->pa[v->k - 2]->child[v->da[v->k - 2]] = w;

                    v->pa[v->k] = v->pa[v->k - 1];
                    v->da[v->k] = 0;
                    v->pa[v->k - 1] = w;
                    v->k++;

                    w = v->pa[v->k - 1]->child[1];
                }

                if ((w->child[0] == NULL || w->child[0]->balance == __BLACK) &&
                    (w->child[1] == NULL || w->child[1]->balance == __BLACK))
                {
                    w->balance = __RED;
                } else {
                    if (w->child[1] == NULL || w->child[1]->balance == __BLACK)
                    {
                        z_tree_node_t *y = w->child[0];
                        y->balance = __BLACK;
                        w->balance = __RED;
                        w->child[0] = y->child[1];
                        y->child[1] = w;
                        w = v->pa[v->k - 1]->child[1] = y;
                    }

                    w->balance = v->pa[v->k - 1]->balance;
                    v->pa[v->k - 1]->balance = __BLACK;
                    w->child[1]->balance = __BLACK;

                    v->pa[v->k - 1]->child[1] = w->child[0];
                    w->child[0] = v->pa[v->k - 1];
                    v->pa[v->k - 2]->child[v->da[v->k - 2]] = w;
                    break;
                }
            } else {
                w = v->pa[v->k - 1]->child[0];

                if (w->balance == __RED) {
                    w->balance = __BLACK;
                    v->pa[v->k - 1]->balance = __RED;

                    v->pa[v->k - 1]->child[0] = w->child[1];
                    w->child[1] = v->pa[v->k - 1];
                    v->pa[v->k - 2]->child[v->da[v->k - 2]] = w;

                    v->pa[v->k] = v->pa[v->k - 1];
                    v->da[v->k] = 1;
                    v->pa[v->k - 1] = w;
                    v->k++;

                    w = v->pa[v->k - 1]->child[0];
                }

                if ((w->child[0] == NULL || w->child[0]->balance == __BLACK) &&
                    (w->child[1] == NULL || w->child[1]->balance == __BLACK))
                {
                    w->balance = __RED;
                } else {
                    if (w->child[0] == NULL || w->child[0]->balance == __BLACK)
                    {
                        z_tree_node_t *y = w->child[1];
                        y->balance = __BLACK;
                        w->balance = __RED;
                        w->child[1] = y->child[0];
                        y->child[0] = w;
                        w = v->pa[v->k - 1]->child[0] = y;
                    }

                    w->balance = v->pa[v->k - 1]->balance;
                    v->pa[v->k - 1]->balance = __BLACK;
                    w->child[0]->balance = __BLACK;

                    v->pa[v->k - 1]->child[0] = w->child[1];
                    w->child[1] = v->pa[v->k - 1];
                    v->pa[v->k - 2]->child[v->da[v->k - 2]] = w;
                    break;
                }
            }

            v->k--;
        }
    }
}

static int __redblack_attach (const z_tree_info_t *tree,
                              z_tree_node_t **root,
                              z_tree_node_t *node)
{
    z_tree_node_t **p;
    struct rb_data v;

    if ((p = __do_insert_lookup(tree, &v, root, node->data)) == NULL)
        return(1);

    *p = node;
    return(__do_insert(tree, &v, root, node));
}

static int __redblack_insert (const z_tree_info_t *tree,
                              z_memory_t *memory,
                              z_tree_node_t **root,
                              void *data)
{
    z_tree_node_t **p;
    struct rb_data v;

    if ((p = __do_insert_lookup(tree, &v, root, data)) == NULL)
        return(1);

    if ((*p = z_memory_struct_alloc(memory, z_tree_node_t)) == NULL)
        return(-1);

    (*p)->data = data;
    return(__do_insert(tree, &v, root, *p));
}

static z_tree_node_t *__redblack_detach (const z_tree_info_t *tree,
                                         z_tree_node_t **root,
                                         const void *key)
{
    struct rb_data v;
    z_tree_node_t *p;
    int cmp, dir;

    v.k = 0;
    p = (z_tree_node_t *)root;
    for (cmp = 1;
         cmp != 0;
         cmp = tree->key_compare(tree->user_data, p->data, key))
    {
        dir = cmp < 0;

        v.pa[v.k] = p;
        v.da[v.k++] = dir;

        if ((p = p->child[dir]) == NULL)
            return(NULL);
    }

    __do_remove(tree, &v, p);

    return(p);
}

static int __redblack_remove (const z_tree_info_t *tree,
                              z_memory_t *memory,
                              z_tree_node_t **root,
                              const void *key)
{
    z_tree_node_t *p;

    if ((p = __redblack_detach(tree, root, key)) == NULL)
        return(1);

    /* Free Node */
    if (tree->data_free != NULL)
        tree->data_free(tree->user_data, p->data);
    z_memory_struct_free(memory, z_tree_node_t, p);

    return(0);
}

z_tree_plug_t z_tree_red_black = {
    .attach = __redblack_attach,
    .detach = __redblack_detach,
    .insert = __redblack_insert,
    .remove = __redblack_remove,
};

