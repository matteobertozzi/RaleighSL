/* File autogenerated, do not edit */
#ifndef _Z_BUILD_CONFIG_H_
#define _Z_BUILD_CONFIG_H_

/* C++ needs to know that types and declarations are C, not C++. */
#ifdef __cplusplus
  #define __Z_BEGIN_DECLS__         extern "C" {
  #define __Z_END_DECLS__           }
#else
  #define __Z_BEGIN_DECLS__
  #define __Z_END_DECLS__
#endif

/* Debugging Mode on! Print as much as you can! */
#define __Z_DEBUG__      1

/* You've support for this things... */
#define Z_CPU_HAS_CRC32C
#define Z_IOPOLL_HAS_EPOLL
#define Z_IOPOLL_HAS_POLL
#define Z_IOPOLL_HAS_SELECT
#define Z_SOCKET_HAS_ACCEPT4
#define Z_SOCKET_HAS_MMSG
#define Z_SOCKET_HAS_UNIX
#define Z_STRING_HAS_MEMCHR
#define Z_STRING_HAS_MEMCMP
#define Z_STRING_HAS_MEMCPY
#define Z_STRING_HAS_MEMMOVE
#define Z_STRING_HAS_MEMRCHR
#define Z_STRING_HAS_MEMSET
#define Z_STRING_HAS_STRCMP
#define Z_STRING_HAS_STRNCMP
#define Z_SYS_HAS_ATOMIC_GCC
#define Z_SYS_HAS_CLOCK_GETTIME
#define Z_SYS_HAS_IFADDRS
#define Z_SYS_HAS_LINUX_MACADDR
#define Z_SYS_HAS_PTHREAD
#define Z_SYS_HAS_PTHREAD_AFFINITY
#define Z_SYS_HAS_PTHREAD_MUTEX
#define Z_SYS_HAS_PTHREAD_RWLOCK
#define Z_SYS_HAS_PTHREAD_SPIN_LOCK
#define Z_SYS_HAS_PTHREAD_THREAD
#define Z_SYS_HAS_PTHREAD_WAIT_COND
#define Z_SYS_HAS_PTHREAD_YIELD

#endif /* !_Z_BUILD_CONFIG_H_ */
