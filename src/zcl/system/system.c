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

#include <sys/sysctl.h>
#include <unistd.h>
#include <stdio.h>

#include <zcl/system.h>

unsigned int z_system_processors (void) {
#if defined(_SC_NPROCESSORS_ONLN)
  long ncore;
  if ((ncore = sysconf(_SC_NPROCESSORS_ONLN)) > 0)
    return(ncore);
#endif
  return(1);
}

uint64_t z_system_memory (void) {
#if defined(__APPLE__)
  size_t size = sizeof(uint64_t);
  uint64_t mem;
  if (!sysctlbyname("hw.memsize", &mem, &size, NULL, 0))
    return(mem);
#elif defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGESIZE)
  long free_pages;
  long page_size;

  free_pages = sysconf(_SC_PHYS_PAGES);
  page_size = sysconf(_SC_PAGESIZE);

  return(free_pages * page_size);
#endif

  return(0);
}

uint64_t z_system_memory_free (void) {
#if defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGESIZE)
  long free_pages;
  long page_size;

  free_pages = sysconf(_SC_AVPHYS_PAGES);
  page_size = sysconf(_SC_PAGESIZE);

  return(free_pages * page_size);
#endif
  return(0);
}

uint64_t z_system_memory_used (void) {
  return(z_system_memory() - z_system_memory_free());
}
