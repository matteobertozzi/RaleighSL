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

static int __avl_insert (z_tree_t *tree, void *data) {
    unsigned char da[Z_TREE_MAX_HEIGHT]; /* Cached comparison results. */
    z_tree_node_t *y, *z;   /* Top node to update balance factor, and parent. */
    z_tree_node_t *p, *q;   /* Iterator, and parent. */
    z_tree_node_t *n;       /* Newly inserted node. */
    z_tree_node_t *w;       /* New root of rebalanced subtree. */
    int k = 0;              /* Number of cached results. */
    int dir;                /* Direction to descend. */
    int cmp;

    z = (z_tree_node_t *) &tree->root;
    y = tree->root;
    dir = 0;
    for (q = z, p = y; p != NULL; q = p, p = p->child[dir]) {
        if (!(cmp = tree->key_compare(tree->user_data, data, p->data))) {
            if (p->data != data && tree->data_free != NULL)
                tree->data_free(tree->user_data, p->data);

            p->data = data;
            return(0);
        }

        if (p->balance != 0)
            z = q, y = p, k = 0;
        da[k++] = dir = cmp > 0;
    }

    n = q->child[dir] = z_object_struct_alloc(tree, z_tree_node_t);
    if (n == NULL)
        return 1;

    tree->size++;
    n->data = data;
    n->child[0] = n->child[1] = NULL;
    n->balance = 0;
    if (y == NULL)
        return 0;

    for (p = y, k = 0; p != n; p = p->child[da[k]], k++) {
        if (da[k] == 0)
            p->balance--;
        else
            p->balance++;
    }

    if (y->balance == -2) {
        z_tree_node_t *x = y->child[0];
        if (x->balance == -1) {
            w = x;
            y->child[0] = x->child[1];
            x->child[1] = y;
            x->balance = y->balance = 0;
        } else {
            w = x->child[1];
            x->child[1] = w->child[0];
            w->child[0] = x;
            y->child[0] = w->child[1];
            w->child[1] = y;
            if (w->balance == -1)
                x->balance = 0, y->balance = +1;
            else if (w->balance == 0)
                x->balance = y->balance = 0;
            else /* |w->balance == +1| */
                x->balance = -1, y->balance = 0;
            w->balance = 0;
        }
    } else if (y->balance == +2) {
        z_tree_node_t *x = y->child[1];
        if (x->balance == +1) {
            w = x;
            y->child[1] = x->child[0];
            x->child[0] = y;
            x->balance = y->balance = 0;
        } else {
            w = x->child[0];
            x->child[0] = w->child[1];
            w->child[1] = x;
            y->child[1] = w->child[0];
            w->child[0] = y;
            if (w->balance == +1)
                x->balance = 0, y->balance = -1;
            else if (w->balance == 0)
                x->balance = y->balance = 0;
            else /* |w->balance == -1| */
                x->balance = +1, y->balance = 0;
            w->balance = 0;
        }
    } else {
        return(0);
    }
    z->child[y != z->child[0]] = w;

    return(0);
}

static int __avl_remove (z_tree_t *tree, const void *key) {
    z_tree_node_t *pa[Z_TREE_MAX_HEIGHT];    /* Nodes. */
    unsigned char da[Z_TREE_MAX_HEIGHT];     /* |avl_link[]| indexes. */
    z_tree_node_t *p;               /* Traverses tree to find node to delete. */
    int cmp;                        /* Result of comparison between |item| and |p|. */
    int k;                          /* Stack pointer. */

    k = 0;
    p = (z_tree_node_t *) &tree->root;
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
        if (r->child[0] == NULL) {
            r->child[0] = p->child[0];
            r->balance = p->balance;
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

            s->child[0] = p->child[0];
            r->child[0] = s->child[1];
            s->child[1] = p->child[1];
            s->balance = p->balance;

            pa[j - 1]->child[da[j - 1]] = s;
            da[j] = 1;
            pa[j] = s;
        }
    }

    /* Free Node */
    if (tree->data_free != NULL)
        tree->data_free(tree->user_data, p->data);

    z_object_struct_free(tree, z_tree_node_t, p);
    tree->size--;

    while (--k > 0) {
        z_tree_node_t *y = pa[k];

        if (da[k] == 0) {
            y->balance++;
            if (y->balance == +1)
                break;
            if (y->balance == +2) {
                z_tree_node_t *x = y->child[1];
                if (x->balance == -1) {
                    z_tree_node_t *w;
                    w = x->child[0];
                    x->child[0] = w->child[1];
                    w->child[1] = x;
                    y->child[1] = w->child[0];
                    w->child[0] = y;
                    if (w->balance == +1)
                        x->balance = 0, y->balance = -1;
                    else if (w->balance == 0)
                        x->balance = y->balance = 0;
                    else /* |w->balance == -1| */
                        x->balance = +1, y->balance = 0;
                    w->balance = 0;
                    pa[k - 1]->child[da[k - 1]] = w;
                } else {
                    y->child[1] = x->child[0];
                    x->child[0] = y;
                    pa[k - 1]->child[da[k - 1]] = x;
                    if (x->balance == 0) {
                        x->balance = -1;
                        y->balance = +1;
                        break;
                    } else {
                        x->balance = y->balance = 0;
                    }
                }
            }
        } else {
            y->balance--;
            if (y->balance == -1)
                break;
            if (y->balance == -2) {
                z_tree_node_t *x = y->child[0];
                if (x->balance == +1) {
                    z_tree_node_t *w;
                    w = x->child[1];
                    x->child[1] = w->child[0];
                    w->child[0] = x;
                    y->child[0] = w->child[1];
                    w->child[1] = y;
                    if (w->balance == -1)
                        x->balance = 0, y->balance = +1;
                    else if (w->balance == 0)
                        x->balance = y->balance = 0;
                    else /* |w->balance == +1| */
                        x->balance = -1, y->balance = 0;
                    w->balance = 0;
                    pa[k - 1]->child[da[k - 1]] = w;
                } else {
                    y->child[0] = x->child[1];
                    x->child[1] = y;
                    pa[k - 1]->child[da[k - 1]] = x;
                    if (x->balance == 0) {
                        x->balance = +1;
                        y->balance = -1;
                        break;
                    } else {
                        x->balance = y->balance = 0;
                    }
                }
            }
        }
    }

    return(0);
}

z_tree_plug_t z_tree_avl = {
    .insert = __avl_insert,
    .remove = __avl_remove,
};

