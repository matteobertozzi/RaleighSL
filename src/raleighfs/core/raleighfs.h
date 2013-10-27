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

#ifndef _RALEIGHFS_H_
#define _RALEIGHFS_H_

#define RALEIGHFS_NAME              "RaleighFS"
#define RALEIGHFS_VERSION           0x050000        /* 0x050102 = 5.1.2 */
#define RALEIGHFS_VERSION_STR       "v5"

#include <raleighfs/errno.h>
#include <raleighfs/plugins.h>
#include <raleighfs/types.h>

#include <raleighfs/filesystem.h>
#include <raleighfs/transaction.h>
#include <raleighfs/semantic.h>
#include <raleighfs/exec.h>

#include <raleighfs/devices/memory.h>

#include <raleighfs/semantics/flat.h>

#include <raleighfs/objects/counter.h>
#include <raleighfs/objects/deque.h>
#include <raleighfs/objects/sset.h>

#endif /* !_RALEIGHFS_H_ */
