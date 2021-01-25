#ifndef HEADER_CARL_WARNLESS_H
#define HEADER_CARL_WARNLESS_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://carl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifdef USE_WINSOCK
#include <carl/carl.h> /* for carl_socket_t */
#endif

#define CARLX_FUNCTION_CAST(target_type, func) \
  (target_type)(void (*) (void))(func)

unsigned short carlx_ultous(unsigned long ulnum);

unsigned char carlx_ultouc(unsigned long ulnum);

int carlx_ultosi(unsigned long ulnum);

int carlx_uztosi(size_t uznum);

carl_off_t carlx_uztoso(size_t uznum);

unsigned long carlx_uztoul(size_t uznum);

unsigned int carlx_uztoui(size_t uznum);

int carlx_sltosi(long slnum);

unsigned int carlx_sltoui(long slnum);

unsigned short carlx_sltous(long slnum);

ssize_t carlx_uztosz(size_t uznum);

size_t carlx_sotouz(carl_off_t sonum);

int carlx_sztosi(ssize_t sznum);

unsigned short carlx_uitous(unsigned int uinum);

size_t carlx_sitouz(int sinum);

#ifdef USE_WINSOCK

int carlx_sktosi(carl_socket_t s);

carl_socket_t carlx_sitosk(int i);

#endif /* USE_WINSOCK */

#if defined(WIN32) || defined(_WIN32)

ssize_t carlx_read(int fd, void *buf, size_t count);

ssize_t carlx_write(int fd, const void *buf, size_t count);

#ifndef BUILDING_WARNLESS_C
#  undef  read
#  define read(fd, buf, count)  carlx_read(fd, buf, count)
#  undef  write
#  define write(fd, buf, count) carlx_write(fd, buf, count)
#endif

#endif /* WIN32 || _WIN32 */

#if defined(__INTEL_COMPILER) && defined(__unix__)

int carlx_FD_ISSET(int fd, fd_set *fdset);

void carlx_FD_SET(int fd, fd_set *fdset);

void carlx_FD_ZERO(fd_set *fdset);

unsigned short carlx_htons(unsigned short usnum);

unsigned short carlx_ntohs(unsigned short usnum);

#endif /* __INTEL_COMPILER && __unix__ */

#endif /* HEADER_CARL_WARNLESS_H */
