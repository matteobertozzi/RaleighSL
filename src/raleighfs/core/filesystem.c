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

#include <raleighfs/raleighfs.h>
#include "private.h"

raleighfs_t *raleighfs_alloc (raleighfs_t *fs) {
  if (__plugin_table_alloc(fs)) {
    return(NULL);
  }

  if (raleighfs_obj_cache_alloc(fs)) {
    __plugin_table_free(fs);
    return(NULL);
  }

  if (raleighfs_txn_mgr_alloc(fs)) {
    __plugin_table_free(fs);
    return(NULL);
  }

  if (raleighfs_semantic_alloc(fs)) {
    __plugin_table_free(fs);
    raleighfs_txn_mgr_free(fs);
    raleighfs_obj_cache_free(fs);
    return(NULL);
  }

  return(fs);
}

void raleighfs_free (raleighfs_t *fs) {
  raleighfs_semantic_free(fs);
  raleighfs_txn_mgr_free(fs);
  raleighfs_obj_cache_free(fs);
  __plugin_table_free(fs);
}

raleighfs_errno_t raleighfs_create (raleighfs_t *fs,
                                    raleighfs_device_t *device,
                                    const raleighfs_format_plug_t *format,
                                    const raleighfs_space_plug_t *space,
                                    const raleighfs_semantic_plug_t *semantic)
{
  raleighfs_errno_t errno;

  /* Initialize file-system struct */
  fs->device = device;
  fs->space.plug = space;
  fs->format.plug = format;
  fs->semantic.plug = semantic;

  /* Create new format */
  if ((errno = __format_call_unrequired(fs, init))) {
    return(errno);
  }

  /* Create new space */
  if ((errno = __space_call_unrequired(fs, init))) {
    __format_call_unrequired(fs, unload);
    return(errno);
  }

  /* Create new semantic layer */
  if ((errno = __semantic_call_unrequired(fs, init))) {
    __space_call_unrequired(fs, unload);
    __format_call_unrequired(fs, unload);
    return(errno);
  }

  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_open (raleighfs_t *fs,
                                  raleighfs_device_t *device)
{
  raleighfs_errno_t errno;

  /* Initialize file-system struct */
  fs->device = device;

  /* Load File-system format */
  if ((errno = __format_call_unrequired(fs, load))) {
    return(errno);
  }

  /* Load space allocator */
  if ((errno = __space_call_unrequired(fs, load))) {
    __format_call_unrequired(fs, unload);
    return(errno);
  }

  /* Load semantic layer */
  if ((errno = __semantic_call_unrequired(fs, load))) {
    __space_call_unrequired(fs, unload);
    __format_call_unrequired(fs, unload);
    return(errno);
  }

  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_close (raleighfs_t *fs) {
  raleighfs_errno_t errno;

  if ((errno = raleighfs_sync(fs)))
    return(errno);

  __semantic_call_unrequired(fs, unload);
  __space_call_unrequired(fs, unload);
  __format_call_unrequired(fs, unload);
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_sync (raleighfs_t *fs) {
#if 0
  raleighfs_errno_t errno;

  if ((errno = __semantic_call_unrequired(fs, sync)))
    return(errno);

  if ((errno = __space_call_unrequired(fs, sync)))
    return(errno);

  if ((errno = __format_call_unrequired(fs, sync)))
    return(errno);

  if ((errno = __device_call_unrequired(fs, sync)))
    return(errno);
#endif
  return(RALEIGHFS_ERRNO_NONE);
}
