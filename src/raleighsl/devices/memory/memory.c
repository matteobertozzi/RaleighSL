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

#include "memory.h"

/* ============================================================================
 *  Memory Device Plugin
 */
const raleighsl_device_plug_t raleighsl_device_memory = {
  .info = {
    .type = RALEIGHSL_PLUG_TYPE_DEVICE,
    .description = "Memory Device",
    .label       = "device-memory",
  },

  .used    = NULL,
  .free    = NULL,
  .sync    = NULL,
  .read    = NULL,
  .write   = NULL,
};
