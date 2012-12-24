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
#define Z_IOPOLL_HAS_EPOLL
#define Z_SOCKET_HAS_ACCEPT4
#define Z_CPU_HAS_CRC32C
#define Z_SOCKET_HAS_UNIX

#endif /* !_Z_BUILD_CONFIG_H_ */
