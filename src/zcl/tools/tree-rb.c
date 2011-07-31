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

static int __redblack_insert (z_tree_t *tree, void *data) {
    z_tree_node_t *pa[Z_TREE_MAX_HEIGHT];/* Nodes on stack. */
    unsigned char da[Z_TREE_MAX_HEIGHT]; /* Directions moved from stack nodes */
    z_tree_node_t *p;           /* Traverses tree looking for insertion point */
    z_tree_node_t *n;           /* Newly inserted node. */
    int k;                      /* Stack height. */

    pa[0] = (z_tree_node_t *) &tree->root;
    da[0] = 0;
    k = 1;
    for (p = tree->root; p != NULL; p = p->child[da[k - 1]]) {
        int cmp = tree->key_compare(tree->user_data, data, p->data);
        if (cmp == 0) {
            if (p->data != data && tree->data_free != NULL)
                tree->data_free(tree->user_data, p->data);

            p->data = data;
            return(0);
        }

        pa[k] = p;
        da[k++] = cmp > 0;
    }

    n = pa[k-1]->child[da[k-1]] = z_object_struct_alloc(tree, z_tree_node_t);
    if (n == NULL)
        return 1;

    n->data = data;
    n->child[0] = n->child[1] = NULL;
    n->balance = __RED;

    while (k >= 3 && pa[k - 1]->balance == __RED) {
        if (da[k - 2] == 0) {
            z_tree_node_t *y = pa[k - 2]->child[1];

            if (y != NULL && y->balance == __RED) {
                pa[k - 1]->balance = y->balance = __BLACK;
                pa[k - 2]->balance = __RED;
                k -= 2;
            } else {
                z_tree_node_t *x;

                if (da[k - 1] == 0) {
                    y = pa[k - 1];
                } else {
                    x = pa[k - 1];
                    y = x->child[1];
                    x->child[1] = y->child[0];
                    y->child[0] = x;
                    pa[k - 2]->child[0] = y;
                }

                x = pa[k - 2];
                x->balance = __RED;
                y->balance = __BLACK;

                x->child[0] = y->child[1];
                y->child[1] = x;
                pa[k - 3]->child[da[k - 3]] = y;
                break;
            }
        } else {
            z_tree_node_t *y = pa[k - 2]->child[0];

            if (y != NULL && y->balance == __RED) {
                pa[k - 1]->balance = y->balance = __BLACK;
                pa[k - 2]->balance = __RED;
                k -= 2;
            } else {
                z_tree_node_t *x;

                if (da[k - 1] == 1) {
                    y = pa[k - 1];
                } else {
                    x = pa[k - 1];
                    y = x->child[0];
                    x->child[0] = y->child[1];
                    y->child[1] = x;
                    pa[k - 2]->child[1] = y;
                }

                x = pa[k - 2];
                x->balance = __RED;
                y->balance = __BLACK;

                x->child[1] = y->child[0];
                y->child[0] = x;
                pa[k - 3]->child[da[k - 3]] = y;
                break;
            }
        }
    }
    tree->root->balance = __BLACK;

    return(0);
}

static int __redblack_remove (z_tree_t *tree, const void *key) {
    z_tree_node_t *pa[Z_TREE_MAX_HEIGHT];/* Nodes on stack. */
    unsigned char da[Z_TREE_MAX_HEIGHT]; /* Directions moved from stack nodes */
    z_tree_node_t *p;    /* The node to delete, or a node part way to it. */
    int cmp;             /* Result of comparison between |item| and |p|. */
    int k;               /* Stack height. */

    k = 0;
    p = (z_tree_node_t *)&tree->root;
    for (cmp = -1;
         cmp != 0;
         cmp = tree->key_compare(tree->user_data, key, p->data))
    {
        int dir = cmp > 0;

        pa[k] = p;
        da[k++] = dir;

        p = p->child[dir];
        if (p == NULL)
            return 1;
    }

    if (p->child[1] == NULL) {
        pa[k - 1]->child[da[k - 1]] = p->child[0];
    } else {
        z_tree_node_t *r = p->child[1];
        int t;

        if (r->child[0] == NULL) {
            r->child[0] = p->child[0];
            t = r->balance;
            r->balance = p->balance;
            p->balance = t;
            pa[k - 1]->child[da[k - 1]] = r;
            da[k] = 1;
            pa[k++] = r;
        } else {
            z_tree_node_t *s;
            int j = k++;

            for (;;) {
                da[k] = 0;
                pa[k++] = r;
                s = r->child[0];
                if (s->child[0] == NULL)
                    break;

                r = s;
            }

            da[j] = 1;
            pa[j] = s;
            pa[j - 1]->child[da[j - 1]] = s;

            s->child[0] = p->child[0];
            r->child[0] = s->child[1];
            s->child[1] = p->child[1];

            t = s->balance;
            s->balance = p->balance;
            p->balance = t;
        }
    }

    if (p->balance == __BLACK) {
        for (;;) {
            z_tree_node_t *x = pa[k - 1]->child[da[k - 1]];
            if (x != NULL && x->balance == __RED) {
                x->balance = __BLACK;
                break;
            }

            if (k < 2)
                break;

            if (da[k - 1] == 0) {
                z_tree_node_t *w = pa[k - 1]->child[1];

                if (w->balance == __RED) {
                    w->balance = __BLACK;
                    pa[k - 1]->balance = __RED;

                    pa[k - 1]->child[1] = w->child[0];
                    w->child[0] = pa[k - 1];
                    pa[k - 2]->child[da[k - 2]] = w;

                    pa[k] = pa[k - 1];
                    da[k] = 0;
                    pa[k - 1] = w;
                    k++;

                    w = pa[k - 1]->child[1];
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
                        w = pa[k - 1]->child[1] = y;
                    }

                    w->balance = pa[k - 1]->balance;
                    pa[k - 1]->balance = __BLACK;
                    w->child[1]->balance = __BLACK;

                    pa[k - 1]->child[1] = w->child[0];
                    w->child[0] = pa[k - 1];
                    pa[k - 2]->child[da[k - 2]] = w;
                    break;
                }
            } else {
                z_tree_node_t *w = pa[k - 1]->child[0];

                if (w->balance == __RED) {
                    w->balance = __BLACK;
                    pa[k - 1]->balance = __RED;

                    pa[k - 1]->child[0] = w->child[1];
                    w->child[1] = pa[k - 1];
                    pa[k - 2]->child[da[k - 2]] = w;

                    pa[k] = pa[k - 1];
                    da[k] = 1;
                    pa[k - 1] = w;
                    k++;

                    w = pa[k - 1]->child[0];
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
                        w = pa[k - 1]->child[0] = y;
                    }

                    w->balance = pa[k - 1]->balance;
                    pa[k - 1]->balance = __BLACK;
                    w->child[0]->balance = __BLACK;

                    pa[k - 1]->child[0] = w->child[1];
                    w->child[1] = pa[k - 1];
                    pa[k - 2]->child[da[k - 2]] = w;
                    break;
                }
            }

            k--;
        }
    }

    /* Free Node */
    if (tree->data_free != NULL)
        tree->data_free(tree->user_data, p->data);

    z_object_struct_free(tree, z_tree_node_t, p);
    tree->size--;

    return(0);
}

z_tree_plug_t z_tree_red_black = {
    .insert = __redblack_insert,
    .remove = __redblack_remove,
};

