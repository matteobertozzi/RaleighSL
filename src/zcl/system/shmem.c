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

#include <zcl/shmem.h>
#include <zcl/debug.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static int __shmem_attach (z_shmem_t *self, int fd, int rdonly) {
  /* grab the memory */
  self->addr = mmap(NULL, self->size,
                    rdonly ? PROT_READ : PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0);
  if (self->addr == MAP_FAILED) {
    Z_LOG_ERRNO_ERROR("mmap()");
    return(-1);
  }

#ifdef F_ADD_SEALS
  /*
   * Make sure the shared memory region will not shrink
   * otherwise someone could cause SIGBUS on us.
   * (see http://lwn.net/Articles/594919/)
   */
  fcntl(self->fd, F_ADD_SEALS, F_SEAL_SHRINK);
#endif
  return(0);
}

int z_shmem_create (z_shmem_t *self, const char *path, uint64_t size) {
  int fd;

  self->path = path;
  self->size = size;
  if ((fd = shm_open(path, O_CREAT | O_EXCL | O_RDWR, 0777)) < 0) {
    Z_LOG_ERRNO_ERROR("shmem_open()");
    return(-1);
  }

  if (ftruncate(fd, size) < 0) {
    Z_LOG_ERRNO_ERROR("ftruncate()");
    goto _creat_fail;
  }

  /* grab the memory */
  if (__shmem_attach(self, fd, 0) < 0) {
    goto _creat_fail;
  }

  close(fd);
  return(0);

_creat_fail:
  close(fd);
  shm_unlink(self->path);
  return(-1);
}

int z_shmem_open (z_shmem_t *self, const char *path, int rdonly) {
  struct stat stbuf;
  int fd;

  self->path = path;
  if ((fd = shm_open(path, rdonly ? O_RDONLY : O_RDWR, 0)) < 0) {
    Z_LOG_ERRNO_ERROR("shmem_open()");
    return(-1);
  }

  /* grab the size */
  if (fstat(fd, &stbuf) < 0) {
    Z_LOG_ERRNO_ERROR("fstat()");
    goto _open_fail;
  }

  self->size = stbuf.st_size;

  /* grab the memory */
  if (__shmem_attach(self, fd, rdonly) < 0) {
    goto _open_fail;
  }

  close(fd);
  return(0);

_open_fail:
  close(fd);
  shm_unlink(self->path);
  return(-1);
}

int z_shmem_close (z_shmem_t *self, int destroy) {
  if (munmap(self->addr, self->size) < 0) {
    Z_LOG_ERRNO_ERROR("munmap()");
    return(-1);
  }
  if (destroy) {
    return shm_unlink(self->path);
  }
  return(0);
}
