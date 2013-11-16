/*
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

#include <zcl/global.h>
#include <zcl/bitmap.h>
#include <zcl/debug.h>

#include "bitmap.h"

#define RALEIGHSL_BITMAP(x)            Z_CAST(raleighsl_bitmap_t, x)

typedef struct raleighsl_bitmap {
} raleighsl_bitmap_t;

/* ============================================================================
 *  PUBLIC Bitmap READ methods
 */
raleighsl_errno_t raleighsl_bitmap_test (raleighsl_t *fs,
                                         const raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         uint64_t offset,
                                         uint64_t count,
                                         int marked)
{
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_bitmap_find (raleighsl_t *fs,
                                         const raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         uint64_t offset,
                                         uint64_t count,
                                         int marked)
{
  return(RALEIGHSL_ERRNO_NONE);
}

/* ============================================================================
 *  PUBLIC Bitmap WRITE methods
 */
raleighsl_errno_t raleighsl_bitmap_mark (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         uint64_t offset,
                                         uint64_t count,
                                         int value)
{
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_bitmap_invert (raleighsl_t *fs,
                                           raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t offset,
                                           uint64_t count)
{
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_bitmap_resize (raleighsl_t *fs,
                                           const raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t count)
{
  return(RALEIGHSL_ERRNO_NONE);
}

/* ============================================================================
 *  Bitmap Object Plugin
 */
static raleighsl_errno_t __object_create (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_bitmap_t *bitmap;

  bitmap = z_memory_struct_alloc(z_global_memory(), raleighsl_bitmap_t);
  if (Z_MALLOC_IS_NULL(bitmap))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  /* TODO */
  object->membufs = bitmap;
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_close (raleighsl_t *fs,
                                         raleighsl_object_t *object)
{
  raleighsl_bitmap_t *bitmap = RALEIGHSL_BITMAP(object->membufs);
  z_memory_struct_free(z_global_memory(), raleighsl_bitmap_t, bitmap);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_commit (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  //raleighsl_bitmap_t *bitmap = RALEIGHSL_BITMAP(object->membufs);
  /* TODO */
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_rollback (raleighsl_t *fs,
                                            raleighsl_object_t *object)
{
  //raleighsl_bitmap_t *bitmap = RALEIGHSL_BITMAP(object->membufs);
  /* TODO */
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_apply (raleighsl_t *fs,
                                         raleighsl_object_t *object,
                                         void *mutation)
{
  //raleighsl_bitmap_t *bitmap = RALEIGHSL_BITMAP(object->membufs);
  /* TODO */
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_revert (raleighsl_t *fs,
                                          raleighsl_object_t *object,
                                          void *mutation)
{
  //raleighsl_bitmap_t *bitmap = RALEIGHSL_BITMAP(object->membufs);
  /* TODO */
  return(RALEIGHSL_ERRNO_NONE);
}

const raleighsl_object_plug_t raleighsl_object_bitmap = {
  .info = {
    .type = RALEIGHSL_PLUG_TYPE_OBJECT,
    .description = "Bitmap Object",
    .label       = "bitmap",
  },

  .create   = __object_create,
  .open     = NULL,
  .close    = __object_close,
  .commit   = __object_commit,
  .rollback = __object_rollback,
  .sync     = NULL,
  .unlink   = NULL,
  .apply    = __object_apply,
  .revert   = __object_revert,
};
