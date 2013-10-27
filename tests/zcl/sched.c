#if 0
#include <zcl/locking.h>

struct raleighfs_sched {
};

struct raleighfs {
  struct raleighfs_sched sched;
};

struct task {
  struct task *next;
  z_rwcsem_op_t operation_type;
};

struct object {
  z_rwcsem_t lock;

  z_spinlock_t wlock;
  struct task *waiting;
};

int object_open (struct object *object) {
  z_rwcsem_init(&(object->lock));
}

void object_close (struct object *object) {
}

int object_exec (struct object *object, struct task *task) {
  if (!z_rwcsem_try_acquire(&(object->lock), task->operation_type)) {
    /* Add the task to the waiting queue */
    z_lock(z_spin, &(object->wlock)) {
      task->next = object->waiting;
      object->waiting = task;
    });
    return(1);
  }

  /* Execute the task */
  /* TODO */

  z_rwcsem_release(&(object->lock), task->operation_type);

  /* Wake up the waiting tasks */
  z_lock(z_spin, &(object->wlock)) {
    /* sched_push_waiting(object->waiting) */
    object->waiting = NULL;
  });
  return(0);
}
#endif
int main (int argc, char **argv) {
  return(0);
}
