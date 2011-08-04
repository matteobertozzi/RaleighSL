#include <unistd.h>
#include <stdio.h>

#include <zcl/workqueue.h>
#include <zcl/memory.h>
#include <zcl/test.h>

#define FUNC_TIME_SLEEP			(1000000 >> 4)
#define JOB_SIZE                (8)

struct user_data {
    z_memory_t memory;
};

static unsigned int __job0_done[JOB_SIZE];
static unsigned int __job1_done[JOB_SIZE];
static unsigned long __job0_count;
static unsigned long __job1_count;

static void __func0 (void *arg) {
    unsigned long idx = (unsigned long)arg;
    __job0_done[idx] = 1;
    usleep(FUNC_TIME_SLEEP);
    __job0_done[idx] = 2;
}

static void __func1 (void *arg) {
    unsigned long idx = (unsigned long)arg;
    __job1_done[idx] = 3;
    usleep(FUNC_TIME_SLEEP);
    __job1_done[idx] = 4;
}

static int __job_fetch (void *arg, void **context, z_invoker_t *func) {
    if (__job0_count == __job1_count && __job0_count == JOB_SIZE)
        return(1);

    if (__job0_count < __job1_count) {
        *func = __func0;
        *context = (void *)__job0_count;
        __job0_count++;
    } else {
        *func = __func1;
        *context = (void *)__job1_count;
        __job1_count++;
    }

    return(0);
}

static int __test_setup (z_test_t *test) {
    unsigned int i;

    __job0_count = 0;
    __job1_count = 0;
    for (i = 0; i < JOB_SIZE; ++i) {
        __job0_done[i] = 0;
        __job1_done[i] = 0;
    }

    return(0);
}

static int __test_queue (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_work_queue_t queue;
    unsigned long i;
    int fail;

    z_work_queue_alloc(&queue, &(data->memory), NULL, NULL);

    for (i = 0; i < JOB_SIZE; ++i) {
        z_work_queue_add(&queue, __func0, (void *)i);
        z_work_queue_add(&queue, __func1, (void *)i);
    }

    z_work_queue_wait(&queue);
    z_work_queue_free(&queue);

    fail = 0;
    for (i = 0; i < JOB_SIZE; ++i) {
        if (__job0_done[i] != 2) {
            printf(" [ !! ] Work Queue Failed Job 0 - %lu %u\n",
                   i, __job0_done[i]);
            fail++;
        }

        if (__job1_done[i] != 4) {
            printf(" [ !! ] Work Queue Failed Job 1 - %lu %u\n",
                   i, __job1_done[i]);
            fail++;
        }
    }

    return(fail);
}

static int __test_fetcher (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_work_queue_t queue;
    unsigned int i;
    int fail;

    z_work_queue_alloc(&queue, &(data->memory), __job_fetch, NULL);
    z_work_queue_wait(&queue);
    z_work_queue_free(&queue);

    fail = 0;
    for (i = 0; i < JOB_SIZE; ++i) {
        if (__job0_done[i] != 2) {
            printf(" [ !! ] Work Queue Fetch Failed Job 0 - %u %u\n",
                   i, __job0_done[i]);
            fail++;
        }

        if (__job1_done[i] != 4) {
            printf(" [ !! ] Work Queue Fetch Failed Job 1 - %u %u\n",
                   i, __job1_done[i]);
            fail++;
        }
    }

    return(fail);
}

static z_test_t __test_work_queue = {
    .setup      = __test_setup,
    .tear_down  = NULL,
    .funcs      = {
        __test_queue,
        __test_fetcher,
        NULL,
    },
};

int main (int argc, char **argv) {
    struct user_data data;
    int res;

    z_memory_init(&(data.memory), z_system_allocator());
    if ((res = z_test_run(&__test_work_queue, &data)))
        printf(" [ !! ] Work Queue %d\n", res);
    else
        printf(" [ ok ] Work Queue\n");
    printf("        - z_work_queue_t        %ubytes\n", (unsigned int)sizeof(z_work_queue_t));
    printf("        - z_work_item_t         %ubytes\n", (unsigned int)sizeof(z_work_item_t));


    return(res);
}

