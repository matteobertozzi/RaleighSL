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

#ifndef _RALEIGHSL_H_
#define _RALEIGHSL_H_

#define RALEIGHSL_NAME              "RaleighSL"
#define RALEIGHSL_VERSION           0x050000        /* 0x050102 = 5.1.2 */
#define RALEIGHSL_VERSION_STR       "v5"

#include <raleighsl/errno.h>
#include <raleighsl/plugins.h>
#include <raleighsl/types.h>

#include <raleighsl/filesystem.h>
#include <raleighsl/transaction.h>
#include <raleighsl/semantic.h>
#include <raleighsl/exec.h>

#include <raleighsl/devices/memory.h>

#include <raleighsl/semantics/flat.h>

#include <raleighsl/objects/number.h>
#include <raleighsl/objects/deque.h>
#include <raleighsl/objects/sset.h>
#include <raleighsl/objects/flow.h>

#endif /* !_RALEIGHSL_H_ */
