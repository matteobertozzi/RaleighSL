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
#include <zcl/debug.h>
#include <zcl/time.h>

#include "private.h"

/* ============================================================================
 *  PUBLIC Journal methods
 */
void raleighsl_journal_add (raleighsl_t *fs, raleighsl_object_t *object) {
  raleighsl_journal_t *journal = &(fs->journal);
  if (z_dlink_is_not_linked(&(object->journal))) {
    z_spin_lock(&(journal->lock));
    z_dlink_add(&(journal->objects), &(object->journal));
    if (!journal->otime) journal->otime = z_time_micros();
    z_spin_unlock(&(journal->lock));
  }
}

void raleighsl_journal_remove (raleighsl_t *fs, raleighsl_object_t *object) {
  raleighsl_journal_t *journal = &(fs->journal);
  if (z_dlink_is_not_linked(&(object->journal))) {
    z_spin_lock(&(journal->lock));
    z_dlink_del(&(object->journal));
    z_spin_unlock(&(journal->lock));
  }
}

#if 0
void raleighsl_journal_sync (raleighsl_t *fs) {
  raleighsl_journal_t *journal = &(fs->journal);
  raleighsl_object_t *object;
  z_dlink_del_for_each_entry(&(journal->objects), object, raleighsl_object_t, journal, {
    raleighsl_object_journal_sync(object);
  });
  journal->otime = 0;
}
#endif
/* ============================================================================
 *  PRIVATE Journal methods
 */
int raleighsl_journal_alloc (raleighsl_t *fs) {
  raleighsl_journal_t *journal = &(fs->journal);
  z_spin_alloc(&(journal->lock));
  z_dlink_init(&(journal->objects));
  journal->otime = 0;
  return(0);
}

void raleighsl_journal_free (raleighsl_t *fs) {
  raleighsl_journal_t *journal = &(fs->journal);
  z_spin_free(&(journal->lock));
}
