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

#include <stdio.h>

#include <zcl/memory.h>
#include <zcl/memset.h>
#include <zcl/memcpy.h>

/* ===========================================================================
 *  Memory Pool
 */
#ifdef Z_MEMORY_USE_POOL
#define PAGE_SIZE       4096

static int z_mempool_init (z_mempool_t *pool) {
    pool->pages = NULL;
    pool->objects = NULL;
    return(0);
}

static int z_mempool_add (z_mempool_t *pool, int size) {
    unsigned int nobjects;
    unsigned int npages;
    z_mempage_t *page;
    z_memobj_t *obj;
    unsigned int i;
    uint8_t *block;

    /* Allocate pool page */
    if ((block = (uint8_t *) malloc(PAGE_SIZE)) == NULL)
        return(1);

    nobjects = PAGE_SIZE / size;
    npages = (size / sizeof(void *)) - 1;

    if (pool->pages == NULL || pool->pages->blocks[npages - 1] != NULL) {
        /* Setup Page Header with link for new block */
        page = (z_mempage_t *)block;
        page->next = pool->pages;
        for (i = 0; i < npages; ++i)
            page->blocks[i] = NULL;
        pool->pages = page;
    } else {
        /* Find a free page to place this full block */
        page = pool->pages;
        for (i = 0; i < npages; ++i) {
            if (page->blocks[i] == NULL) {
                page->blocks[i] = block;
                break;
            }
        }

        /* This is not a page, is a full block! */
        page = NULL;
    }

    /* Split Objects */
    for (i = !!(page != NULL); i < nobjects; ++i) {
        obj = (z_memobj_t *)(block + (i * size));
        obj->next = pool->objects;
        obj->used = 0;
        pool->objects = obj;
    }

    return(0);
}

static int z_mempool_remove (z_mempool_t *pool, int size) {
    return(1);
}

static int z_mempool_in (z_mempool_t *pool, int size, const uint8_t *ptr) {
    unsigned int i, npages;
    z_mempage_t *page;
    const uint8_t *p;

    npages = (size / sizeof(void *)) - 1;
    for (page = pool->pages; page != NULL; page = page->next) {
        p = (const uint8_t *)page;

        if (ptr >= p && ptr < (p + PAGE_SIZE))
            return(1);

        for (i = 0; i < npages; ++i) {
            if ((p = page->blocks[i]) == NULL)
                continue;

            if (ptr >= p && ptr < (p + PAGE_SIZE))
                return(1);
        }
    }

    return(0);
}
#endif /* Z_MEMORY_USE_POOL */

/* ===========================================================================
 *  Public Memory
 */
//#define Z_MEMORY_USE_POOL
//#define Z_MEMORY_USE_POOL_PREALLOC
void z_memory_init (z_memory_t *memory, z_allocator_t *allocator) {
#ifdef Z_MEMORY_USE_POOL
    unsigned int size;
    unsigned int i;
#endif
    memory->allocator = allocator;

#ifdef Z_MEMORY_USE_POOL
    for (i = 0; i < Z_MEMORY_POOL_SIZE; ++i)
        z_mempool_init(&(memory->pool[i]));

#ifdef Z_MEMORY_USE_POOL_PREALLOC
    size = 1 << Z_MEMORY_POOL_SHIFT;
    for (i = 0; i < 3; ++i) {
        z_mempool_add(&(memory->pool[i]), size);
        size += 1 << Z_MEMORY_POOL_SHIFT;
    }

    for (i = 0; i < 392; ++i)
        z_mempool_add(&(memory->pool[0]), 16);
#endif /* Z_MEMORY_USE_POOL_PREALLOC */
#endif
}

void *z_memory_zalloc (z_memory_t *memory, unsigned int size) {
    void *ptr;

    if ((ptr = z_memory_alloc(memory, size)) != NULL)
        z_memzero(ptr, size);

    return(ptr);
}

void *__z_memory_dup (z_memory_t *memory, const void *src, unsigned int size) {
    void *ptr;

    if ((ptr = z_memory_alloc(memory, size)) != NULL)
        z_memcpy(ptr, src, size);

    return(ptr);
}

void *z_memory_pool_pop (z_memory_t *memory, unsigned int size) {
#ifdef Z_MEMORY_USE_POOL
    unsigned int block_size;
    unsigned int n;

    block_size = z_align_up(size, 1 << Z_MEMORY_POOL_SHIFT);
    n = (block_size >> Z_MEMORY_POOL_SHIFT);

    if (n < Z_MEMORY_POOL_SIZE) {
        z_mempool_t *node = &(memory->pool[n - 1]);
        if (node->objects != NULL) {
            z_memobj_t *obj;
            obj = node->objects;
            node->objects = obj->next;
            return(obj);
        }
    }
    printf("CACHE MISS %u n %u\n", block_size, n);
#endif
    return(z_memory_alloc(memory, size));
}

void *z_memory_pool_zpop (z_memory_t *memory, unsigned int size) {
    void *ptr;

    if ((ptr = z_memory_pool_pop(memory, size)) != NULL)
        z_memzero(ptr, size);

    return(ptr);
}

void z_memory_pool_push (z_memory_t *memory, unsigned int size, void *ptr) {
#ifdef Z_MEMORY_USE_POOL
    unsigned int block_size;
    unsigned int n;

    block_size = z_align_up(size, 1 << Z_MEMORY_POOL_SHIFT);
    n = (block_size >> Z_MEMORY_POOL_SHIFT);

    if (n < Z_MEMORY_POOL_SIZE) {
        z_mempool_t *node = &(memory->pool[n - 1]);
        if (z_mempool_in(node, block_size, (const uint8_t *)ptr)) {
            z_memobj_t *obj = (z_memobj_t *)ptr;
            obj->next = node->objects;
            node->objects = obj;
            return;
        }
    }
#endif
    z_memory_free(memory, ptr);
}

