/*
 *   Copyright 2007-2012 Matteo Bertozzi
 *
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

#ifndef FUSE_USE_VERSION
  #define FUSE_USE_VERSION 29
#endif

#ifndef _POSIX_C_SOURCE
  #define _POSIX_C_SOURCE  200809L
#endif

#ifndef _XOPEN_SOURCE
  #define _XOPEN_SOURCE 700
#endif

#include <fuse.h>

#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_SETXATTR
    #include <sys/xattr.h>
#endif

/* ============================================================================
 *  File-System helper struct
 */
struct r5l_fuse {
  const char * root;
  unsigned int root_length;
};

static struct r5l_fuse __r5l;

static int r5l_fuse_open (void) {
  return(0);
}

static void r5l_fuse_close (void) {
}

/* ============================================================================
 *  FUSE operations
 */
static int __fuse_getattr (const char *path, struct stat *stbuf) {
  return(-1);
}

static int __fuse_readlink (const char *path, char *buf, size_t size) {
  return(-1);
}

static int __fuse_readdir (const char *path,
                           void *buf,
                           fuse_fill_dir_t filler,
                           off_t offset,
                           struct fuse_file_info *fi)
{
  return(-1);
}

static int __fuse_mknod (const char *path, mode_t mode, dev_t rdev) {
  return(-1);
}

static int __fuse_mkdir (const char *path, mode_t mode) {
  return(-1);
}

static int __fuse_unlink (const char *path) {
  return(-1);
}

static int __fuse_rmdir (const char *path) {
  return(-1);
}

static int __fuse_symlink (const char *from, const char *to) {
  return(-1);
}

static int __fuse_rename (const char *from, const char *to) {
  return(-1);
}

static int __fuse_link (const char *from, const char *to) {
  return(-1);
}

static int __fuse_chmod (const char *path, mode_t mode) {
  return(-1);
}

static int __fuse_chown (const char *path, uid_t uid, gid_t gid) {
  return(-1);
}

static int __fuse_truncate (const char *path, off_t size) {
  return(-1);
}

static int __fuse_utimens (const char *path, const struct timespec ts[2]) {
  return(-1);
}

static int __fuse_create (const char *path, mode_t mode, struct fuse_file_info *fi) {
  return(-1);
}

static int __fuse_open (const char *path, struct fuse_file_info *fi) {
  return(-1);
}

static int __fuse_read (const char *path,
                        char *buf,
                        size_t size,
                        off_t offset,
                        struct fuse_file_info *fi)
{
  return(-1);
}

static int __fuse_write (const char *path,
                         const char *buf,
                         size_t size,
                         off_t offset,
                         struct fuse_file_info *fi)
{
  return(-1);
}

static int __fuse_statfs (const char *path, struct statvfs *stbuf) {
  return(-1);
}

static int __fuse_release (const char *path, struct fuse_file_info *fi) {
  return(-1);
}

static int __fuse_fsync (const char *path,
                         int isdatasync,
                         struct fuse_file_info *fi)
{
  return(-1);
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int __fuse_setxattr (const char *path,
                            const char *name,
                            const char *value,
                            size_t size,
                            int flags)
{
  return(-1);
}

static int __fuse_getxattr (const char *path,
                            const char *name,
                            char *value,
                            size_t size)
{
  return(-1);
}

static int __fuse_listxattr (const char *path, char *list, size_t size) {
  return(-1);
}

static int __fuse_removexattr (const char *path, const char *name) {
  return(-1);
}
#endif /* HAVE_SETXATTR */

/* ============================================================================
 *  FUSE operations table
 */
static struct fuse_operations __r5l_fuse_ops = {
  .getattr     = __fuse_getattr,
  .readlink    = __fuse_readlink,
  .readdir     = __fuse_readdir,
  .mknod       = __fuse_mknod,
  .mkdir       = __fuse_mkdir,
  .symlink     = __fuse_symlink,
  .unlink      = __fuse_unlink,
  .rmdir       = __fuse_rmdir,
  .rename      = __fuse_rename,
  .link        = __fuse_link,
  .chmod       = __fuse_chmod,
  .chown       = __fuse_chown,
  .truncate    = __fuse_truncate,
  .utimens     = __fuse_utimens,
  .open        = __fuse_open,
  .create      = __fuse_create,
  .read        = __fuse_read,
  .write       = __fuse_write,
  .statfs      = __fuse_statfs,
  .release     = __fuse_release,
  .fsync       = __fuse_fsync,
#ifdef HAVE_SETXATTR
  .setxattr    = __fuse_setxattr,
  .getxattr    = __fuse_getxattr,
  .listxattr   = __fuse_listxattr,
  .removexattr = __fuse_removexattr,
#endif
};


/* ============================================================================
 *  R5L_FUSE-FUSE Main
 */
enum {
  R5L_FUSE_KEY_HELP,
  R5L_FUSE_KEY_VERSION,
};

#define R5L_FUSE_OPT(t, p, v)   { t, __builtin_offsetof(struct r5l_fuse, p), v }

static struct fuse_opt __r5l_opts[] = {
  R5L_FUSE_OPT("root=%s", root, 0),

  FUSE_OPT_KEY("-V", R5L_FUSE_KEY_VERSION),
  FUSE_OPT_KEY("--version", R5L_FUSE_KEY_VERSION),
  FUSE_OPT_KEY("-h", R5L_FUSE_KEY_HELP),
  FUSE_OPT_KEY("--help", R5L_FUSE_KEY_HELP),
  FUSE_OPT_END
};

static int __r5l_opt_proc (void *data,
                             const char *arg,
                             int key,
                             struct fuse_args *outargs)
{
  switch (key) {
    case R5L_FUSE_KEY_HELP:
      fprintf(stderr,
              "usage: %s mountpoint --root rootpath [options]\n"
              "\n"
              "general options:\n"
              "    -o opt,[opt...]    mount options\n"
              "    -h   --help        print help\n"
              "    -V   --version     print version\n"
              "\n"
              "R5L_FUSE options:\n"
              "    -o root=ROOT-PATH  root path\n", outargs->argv[0]);
      fuse_opt_add_arg(outargs, "-ho");
      fuse_main(outargs->argc, outargs->argv, &__r5l_fuse_ops, NULL);
      exit(EXIT_FAILURE);

    case R5L_FUSE_KEY_VERSION:
      fprintf(stderr, "R5L_FUSE version 0.1\n");
      fuse_opt_add_arg(outargs, "--version");
      fuse_main(outargs->argc, outargs->argv, &__r5l_fuse_ops, NULL);
      exit(EXIT_SUCCESS);
  }
  return(1);
}

int main (int argc, char **argv) {
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  int res;

  __r5l.root = NULL;
  __r5l.root_length = 0;

  fuse_opt_parse(&args, &__r5l, __r5l_opts, __r5l_opt_proc);
  __r5l.root_length = (__r5l.root != NULL) ? strlen(__r5l.root) : 0;
  if (!__r5l.root_length) {
    fprintf(stderr, "r5l: root path parameter (add -o root=path)\n");
    return(EXIT_FAILURE);
  }

  if (__r5l.root[__r5l.root_length - 1] == '/')
    __r5l.root_length--;

  if ((r5l_fuse_open()) < 0)
    return(EXIT_FAILURE);

  res = fuse_main(args.argc, args.argv, &__r5l_fuse_ops, NULL);

  r5l_fuse_close();
  return(res);
}
