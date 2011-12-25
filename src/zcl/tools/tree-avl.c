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

struct avl_insert_data {
    unsigned char da[Z_TREE_MAX_HEIGHT]; /* Cached comparison results. */
    z_tree_node_t *y, *z;   /* Top node to update balance factor, and parent. */
};

struct avl_remove_data {
    z_tree_node_t *pa[Z_TREE_MAX_HEIGHT];    /* Nodes. */
    unsigned char da[Z_TREE_MAX_HEIGHT];     /* |avl_link[]| indexes. */
    int k;                                   /* Stack pointer. */
};

static z_tree_node_t **__do_insert_lookup (const z_tree_info_t *tree,
                                           struct avl_insert_data *v,
                                           z_tree_node_t **root,
                                           void *data)
{
    z_tree_node_t *p, *q;
    int k, dir, cmp;

    v->z = (z_tree_node_t *)root;
    v->y = *root;
    dir = 0;
    k = 0;
    for (q = v->z, p = v->y; p != NULL; q = p, p = p->child[dir]) {
        if (!(cmp = tree->key_compare(tree->user_data, p->data, data))) {
            if (p->data != data && tree->data_free != NULL)
                tree->data_free(tree->user_data, p->data);

            p->data = data;
            return(NULL);
        }

        if (p->balance != 0)
            v->z = q, v->y = p, k = 0;
        v->da[k++] = dir = cmp < 0;
    }

    return(&(q->child[dir]));
}

static int __do_insert (const z_tree_info_t *tree,
                        const struct avl_insert_data *v,
                        z_tree_node_t *n)
{
    z_tree_node_t *w;
    z_tree_node_t *p;
    int k;

    n->child[0] = n->child[1] = NULL;
    n->balance = 0;
    if (v->y == NULL)
        return 0;

    for (p = v->y, k = 0; p != n; p = p->child[v->da[k]], k++) {
        if (v->da[k] == 0)
            p->balance--;
        else
            p->balance++;
    }

    if (v->y->balance == -2) {
        z_tree_node_t *x = v->y->child[0];
        if (x->balance == -1) {
            w = x;
            v->y->child[0] = x->child[1];
            x->child[1] = v->y;
            x->balance = v->y->balance = 0;
        } else {
            w = x->child[1];
            x->child[1] = w->child[0];
            w->child[0] = x;
            v->y->child[0] = w->child[1];
            w->child[1] = v->y;
            if (w->balance == -1)
                x->balance = 0, v->y->balance = +1;
            else if (w->balance == 0)
                x->balance = v->y->balance = 0;
            else /* |w->balance == +1| */
                x->balance = -1, v->y->balance = 0;
            w->balance = 0;
        }
    } else if (v->y->balance == +2) {
        z_tree_node_t *x = v->y->child[1];
        if (x->balance == +1) {
            w = x;
            v->y->child[1] = x->child[0];
            x->child[0] = v->y;
            x->balance = v->y->balance = 0;
        } else {
            w = x->child[0];
            x->child[0] = w->child[1];
            w->child[1] = x;
            v->y->child[1] = w->child[0];
            w->child[0] = v->y;
            if (w->balance == +1)
                x->balance = 0, v->y->balance = -1;
            else if (w->balance == 0)
                x->balance = v->y->balance = 0;
            else /* |w->balance == -1| */
                x->balance = +1, v->y->balance = 0;
            w->balance = 0;
        }
    } else {
        return(0);
    }
    v->z->child[v->y != v->z->child[0]] = w;

    return(0);
}


static void __do_remove (const z_tree_info_t *tree,
                         struct avl_remove_data *v,
                         z_tree_node_t *p)
{
    int k = v->k;

    if (p->child[1] == NULL) {
        v->pa[k - 1]->child[v->da[k - 1]] = p->child[0];
    } else {
        z_tree_node_t *r = p->child[1];
        if (r->child[0] == NULL) {
            r->child[0] = p->child[0];
            r->balance = p->balance;
            v->pa[k - 1]->child[v->da[k - 1]] = r;
            v->da[k] = 1;
            v->pa[k++] = r;
        } else {
            z_tree_node_t *s;
            int j = k++;

            for (;;) {
                v->da[k] = 0;
                v->pa[k++] = r;
                s = r->child[0];
                if (s->child[0] == NULL)
                    break;

                r = s;
            }

            s->child[0] = p->child[0];
            r->child[0] = s->child[1];
            s->child[1] = p->child[1];
            s->balance = p->balance;

            v->pa[j - 1]->child[v->da[j - 1]] = s;
            v->da[j] = 1;
            v->pa[j] = s;
        }
    }

    while (--k > 0) {
        z_tree_node_t *y = v->pa[k];

        if (v->da[k] == 0) {
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
                    v->pa[k - 1]->child[v->da[k - 1]] = w;
                } else {
                    y->child[1] = x->child[0];
                    x->child[0] = y;
                    v->pa[k - 1]->child[v->da[k - 1]] = x;
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
                    v->pa[k - 1]->child[v->da[k - 1]] = w;
                } else {
                    y->child[0] = x->child[1];
                    x->child[1] = y;
                    v->pa[k - 1]->child[v->da[k - 1]] = x;
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
}

static int __avl_attach (const z_tree_info_t *tree,
                         z_tree_node_t **root,
                         z_tree_node_t *node)
{
    struct avl_insert_data v;
    z_tree_node_t **p;

    if ((p = __do_insert_lookup(tree, &v, root, node->data)) == NULL)
        return(1);

    *p = node;
    return(__do_insert(tree, &v, node));
}

static z_tree_node_t *__avl_detach (const z_tree_info_t *tree,
                                    z_tree_node_t **root,
                                    const void *key)
{
    struct avl_remove_data v;
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

static int __avl_insert (const z_tree_info_t *tree,
                         z_memory_t *memory,
                         z_tree_node_t **root,
                         void *data)
{
    struct avl_insert_data v;
    z_tree_node_t **p;

    if ((p = __do_insert_lookup(tree, &v, root, data)) == NULL)
        return(1);

    if ((*p = z_memory_struct_alloc(memory, z_tree_node_t)) == NULL)
        return(-1);

    (*p)->data = data;
    return(__do_insert(tree, &v, *p));
}

static int __avl_remove (const z_tree_info_t *tree,
                         z_memory_t *memory,
                         z_tree_node_t **root,
                         const void *key)
{
    z_tree_node_t *p;

    if ((p = __avl_detach(tree, root, key)) == NULL)
        return(1);

    /* Free Node */
    if (tree->data_free != NULL)
        tree->data_free(tree->user_data, p->data);
    z_memory_struct_free(memory, z_tree_node_t, p);

    return(0);
}

z_tree_plug_t z_tree_avl = {
    .attach = __avl_attach,
    .detach = __avl_detach,
    .insert = __avl_insert,
    .remove = __avl_remove,
};

