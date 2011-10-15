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

#include <zcl/memory.h>
#include <zcl/memset.h>
#include <zcl/memcpy.h>
#include <zcl/debug.h>

#define USE_MEMORY_POOL             0
#define MMPOOL_CHECKS               1       /* This is slow! */

/* ===========================================================================
 *  Memory Pool
 */
typedef struct mmpage z_memory_page;

#define PAGE_SIZE               (8 * 1024)
#define PAGE_HEAD_SIZE          (sizeof(struct mmpage) - sizeof(uint8_t))
#define MMOBJECT_SIZE           sizeof(struct mmobject)
#define MMOBJECT_INFO_SIZE      sizeof(uint16_t)

#define MMOBJECT_FROM_PTR(x)    MMOBJECT(((uint8_t *)(x)) - MMOBJECT_INFO_SIZE)
#define MMOBJECT_TO_PTR(x)      (MMOBJECT(x)->data.blob)
#define MMOBJECT(x)             ((struct mmobject *)(x))
#define MMPAGE(x)               ((struct mmpage *)(x))
#define MMPOOL(x)               ((struct mmpool *)(x))

#define __mmpage_contains(page, ptr)                                        \
    (((uint8_t *)(ptr)) > ((uint8_t *)(page)) &&                            \
     ((uint8_t *)(ptr)) < (((uint8_t *)(page)) + PAGE_SIZE))

struct mmobject {
    uint16_t index;
    union {
        struct mmobject *next;
        uint8_t blob[1];
    } data;
} __attribute__((packed));

struct mmpage {
    struct mmpage   *prev;
    struct mmpage   *next;
    struct mmobject *head;
    uint16_t used;
    uint8_t  blob[1];
} __attribute__((packed));

struct mmpool {
    struct mmpage *head;
    struct mmpage *tail;
};

/*
 * +------+----+----+----+----+-----+----+
 * | head | o0 | o1 | o2 | o3 | ... | oN |
 * +------+----+----+----+----+-----+----+
 *    |___________^
 */

/* Allocate new memory page and return the first object */
void *__mmpage_push (struct mmpool *pool, unsigned int size) {
    struct mmobject *object_next;
    unsigned int i, nobjects;
    struct mmobject *object;
    struct mmpage *page;

    /* Allocate new page */
    if ((page = (struct mmpage *)malloc(PAGE_SIZE)) == NULL)
        return(NULL);

    /* Add page to pool */
    if (pool->head != NULL) {
        pool->head->prev = page;
        page->next = pool->head;
        page->prev = NULL;
    } else {
        page->next = NULL;
        page->prev = NULL;
        pool->tail = page;
    }
    pool->head = page;

    /* Initialize Head Objects */
    page->used = 1;
    page->head = MMOBJECT(page->blob + size);

    /* Initialize Object links */
    object = MMOBJECT(page->blob);
    nobjects = ((PAGE_SIZE - PAGE_HEAD_SIZE) / size) - 1;
    for (i = 0; i < nobjects; ++i) {
        object_next = MMOBJECT(((uint8_t *)object) + size);
        object->index = (uint16_t)(i & 0xffff);
        object->data.next = object_next;
        object = object_next;
    }
    object->index = (uint16_t)(i & 0xffff);
    object->data.next = NULL;

    return(MMOBJECT_TO_PTR(page->blob));
}

void __mmpool_alloc (struct mmpool *pool) {
    pool->head = NULL;
    pool->tail = NULL;
}

void __mmpool_free (struct mmpool *pool) {
    struct mmpage *pnext;
    struct mmpage *page;

    for (page = pool->head; page != NULL; page = pnext) {
        pnext = page->next;
        free(page);
    }

    pool->head = NULL;
    pool->tail = NULL;
}

void *__mmpool_pop (struct mmpool *pool, unsigned int size) {
    struct mmobject *object;
    struct mmpage *page;

    size += MMOBJECT_INFO_SIZE;

    if ((page = pool->head) != NULL) {
        if ((object = page->head) != NULL) {
            page->used++;
            page->head = object->data.next;
        }

        /* No more blocks here, but somewhere.. maybe.. */
        if (page->head == NULL && pool->head != pool->tail) {
            /* Move this page to tail */
            pool->head = page->next;
            pool->head->prev = NULL;
            pool->tail->next = page;
            page->prev = pool->tail;
            page->next = NULL;
            pool->tail = page;

            /* Try to bring someone with space up */
            if ((page = pool->head) == NULL) {
                for (page = page->next; page != pool->tail; page = page->next) {
                    if (page->head == NULL)
                        continue;

                    page->prev->next = page->next;
                    page->next->prev = page->prev;
                    pool->head->prev = page;
                    page->next = pool->head;
                    page->prev = NULL;
                    pool->head = page;
                    break;
                }
            }
        }

        if (object != NULL)
            return(MMOBJECT_TO_PTR(object));
    }

#if MMPOOL_CHECKS
    unsigned int i;
    for (i = 0, page = pool->head; page != NULL; page = page->next, ++i) {
        if (page->head != NULL)
            Z_PRINT_DEBUG("WARN: There's one page %d free you're allocating another\n", i);
    }
#endif /* MMPOOL_CHECKS */

    return(__mmpage_push(pool, size));
}

void __mmpool_push (struct mmpool *pool, void *ptr, unsigned int size) {
    struct mmobject *object;
    struct mmpage *page;

    size += MMOBJECT_INFO_SIZE;

    object = MMOBJECT_FROM_PTR(ptr);
    page = MMPAGE(((uint8_t *)object) - (object->index*size) - PAGE_HEAD_SIZE);

    if (--(page->used) == 0 && (pool->head != pool->tail)) {
        /* Release Page */
        if (page == pool->head) {
            pool->head = page->next;
            pool->head->prev = NULL;
        } else if (page == pool->tail) {
            pool->tail = page->prev;
            pool->tail->next = NULL;
        } else {
            page->prev->next = page->next;
            page->next->prev = page->prev;
        }

        free(page);
    } else {
        /* Release Object */
        object->data.next = page->head;
        page->head = object;

        if (page != pool->head) {
            if (page == pool->tail) {
                pool->tail = page->prev;
                pool->tail->next = NULL;
            } else {
                page->prev->next = page->next;
                page->next->prev = page->prev;
            }

            pool->head->prev = page;
            page->next = pool->head;
            page->prev = NULL;
            pool->head = page;
        }
    }
}


/* ===========================================================================
 *  Public Memory
 */

void z_memory_init (z_memory_t *memory, z_allocator_t *allocator) {
    memory->allocator = allocator;
#if USE_MEMORY_POOL
    /* Initialize pool for 16/24/32 */
    __mmpool_alloc(MMPOOL(&memory->pool[0]));
    __mmpool_alloc(MMPOOL(&memory->pool[1]));
    __mmpool_alloc(MMPOOL(&memory->pool[2]));
#endif
}

void *z_memory_zalloc (z_memory_t *memory, unsigned int size) {
    void *ptr;

    ptr = z_memory_alloc(memory, size);
    if (Z_LIKELY(ptr != NULL))
        z_memzero(ptr, size);

    return(ptr);
}

void *__z_memory_dup (z_memory_t *memory, const void *src, unsigned int size) {
    void *ptr;

    ptr = z_memory_alloc(memory, size);
    if (Z_LIKELY(ptr != NULL))
        z_memcpy(ptr, src, size);

    return(ptr);
}

void *z_memory_pool_pop (z_memory_t *memory, unsigned int size) {
#if USE_MEMORY_POOL
    void *pool;

    switch (size) {
        case 16: pool = &memory->pool[0]; break;
        case 24: pool = &memory->pool[1]; break;
        case 32: pool = &memory->pool[2]; break;
        default: pool = NULL; break;
    }

    if (pool != NULL)
        return(__mmpool_pop(MMPOOL(pool), size));
#endif

    return(z_memory_alloc(memory, size));
}

void *z_memory_pool_zpop (z_memory_t *memory, unsigned int size) {
    void *ptr;

    ptr = z_memory_pool_pop(memory, size);
    if (Z_LIKELY(ptr != NULL))
        z_memzero(ptr, size);

    return(ptr);
}

void z_memory_pool_push (z_memory_t *memory, unsigned int size, void *ptr) {
#if USE_MEMORY_POOL
    void *pool;

    switch (size) {
        case 16: pool = &memory->pool[0]; break;
        case 24: pool = &memory->pool[1]; break;
        case 32: pool = &memory->pool[2]; break;
        default: pool = NULL; break;
    }

    if (pool != NULL)
        __mmpool_push(MMPOOL(pool), ptr, size);
    else
#endif
        z_memory_free(memory, ptr);
}

