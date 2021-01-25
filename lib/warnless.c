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

#include "carl_setup.h"

#if defined(__INTEL_COMPILER) && defined(__unix__)

#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif

#endif /* __INTEL_COMPILER && __unix__ */

#define BUILDING_WARNLESS_C 1

#include "warnless.h"

#define CARL_MASK_SCHAR  0x7F
#define CARL_MASK_UCHAR  0xFF

#if (SIZEOF_SHORT == 2)
#  define CARL_MASK_SSHORT  0x7FFF
#  define CARL_MASK_USHORT  0xFFFF
#elif (SIZEOF_SHORT == 4)
#  define CARL_MASK_SSHORT  0x7FFFFFFF
#  define CARL_MASK_USHORT  0xFFFFFFFF
#elif (SIZEOF_SHORT == 8)
#  define CARL_MASK_SSHORT  0x7FFFFFFFFFFFFFFF
#  define CARL_MASK_USHORT  0xFFFFFFFFFFFFFFFF
#else
#  error "SIZEOF_SHORT not defined"
#endif

#if (SIZEOF_INT == 2)
#  define CARL_MASK_SINT  0x7FFF
#  define CARL_MASK_UINT  0xFFFF
#elif (SIZEOF_INT == 4)
#  define CARL_MASK_SINT  0x7FFFFFFF
#  define CARL_MASK_UINT  0xFFFFFFFF
#elif (SIZEOF_INT == 8)
#  define CARL_MASK_SINT  0x7FFFFFFFFFFFFFFF
#  define CARL_MASK_UINT  0xFFFFFFFFFFFFFFFF
#elif (SIZEOF_INT == 16)
#  define CARL_MASK_SINT  0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
#  define CARL_MASK_UINT  0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
#else
#  error "SIZEOF_INT not defined"
#endif

#if (SIZEOF_LONG == 2)
#  define CARL_MASK_SLONG  0x7FFFL
#  define CARL_MASK_ULONG  0xFFFFUL
#elif (SIZEOF_LONG == 4)
#  define CARL_MASK_SLONG  0x7FFFFFFFL
#  define CARL_MASK_ULONG  0xFFFFFFFFUL
#elif (SIZEOF_LONG == 8)
#  define CARL_MASK_SLONG  0x7FFFFFFFFFFFFFFFL
#  define CARL_MASK_ULONG  0xFFFFFFFFFFFFFFFFUL
#elif (SIZEOF_LONG == 16)
#  define CARL_MASK_SLONG  0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFL
#  define CARL_MASK_ULONG  0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFUL
#else
#  error "SIZEOF_LONG not defined"
#endif

#if (SIZEOF_CARL_OFF_T == 2)
#  define CARL_MASK_SCOFFT  CARL_OFF_T_C(0x7FFF)
#  define CARL_MASK_UCOFFT  CARL_OFF_TU_C(0xFFFF)
#elif (SIZEOF_CARL_OFF_T == 4)
#  define CARL_MASK_SCOFFT  CARL_OFF_T_C(0x7FFFFFFF)
#  define CARL_MASK_UCOFFT  CARL_OFF_TU_C(0xFFFFFFFF)
#elif (SIZEOF_CARL_OFF_T == 8)
#  define CARL_MASK_SCOFFT  CARL_OFF_T_C(0x7FFFFFFFFFFFFFFF)
#  define CARL_MASK_UCOFFT  CARL_OFF_TU_C(0xFFFFFFFFFFFFFFFF)
#elif (SIZEOF_CARL_OFF_T == 16)
#  define CARL_MASK_SCOFFT  CARL_OFF_T_C(0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)
#  define CARL_MASK_UCOFFT  CARL_OFF_TU_C(0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)
#else
#  error "SIZEOF_CARL_OFF_T not defined"
#endif

#if (SIZEOF_SIZE_T == SIZEOF_SHORT)
#  define CARL_MASK_SSIZE_T  CARL_MASK_SSHORT
#  define CARL_MASK_USIZE_T  CARL_MASK_USHORT
#elif (SIZEOF_SIZE_T == SIZEOF_INT)
#  define CARL_MASK_SSIZE_T  CARL_MASK_SINT
#  define CARL_MASK_USIZE_T  CARL_MASK_UINT
#elif (SIZEOF_SIZE_T == SIZEOF_LONG)
#  define CARL_MASK_SSIZE_T  CARL_MASK_SLONG
#  define CARL_MASK_USIZE_T  CARL_MASK_ULONG
#elif (SIZEOF_SIZE_T == SIZEOF_CARL_OFF_T)
#  define CARL_MASK_SSIZE_T  CARL_MASK_SCOFFT
#  define CARL_MASK_USIZE_T  CARL_MASK_UCOFFT
#else
#  error "SIZEOF_SIZE_T not defined"
#endif

/*
** unsigned long to unsigned short
*/

unsigned short carlx_ultous(unsigned long ulnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(ulnum <= (unsigned long) CARL_MASK_USHORT);
  return (unsigned short)(ulnum & (unsigned long) CARL_MASK_USHORT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** unsigned long to unsigned char
*/

unsigned char carlx_ultouc(unsigned long ulnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(ulnum <= (unsigned long) CARL_MASK_UCHAR);
  return (unsigned char)(ulnum & (unsigned long) CARL_MASK_UCHAR);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** unsigned long to signed int
*/

int carlx_ultosi(unsigned long ulnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(ulnum <= (unsigned long) CARL_MASK_SINT);
  return (int)(ulnum & (unsigned long) CARL_MASK_SINT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** unsigned size_t to signed carl_off_t
*/

carl_off_t carlx_uztoso(size_t uznum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4310) /* cast truncates constant value */
#endif

  DEBUGASSERT(uznum <= (size_t) CARL_MASK_SCOFFT);
  return (carl_off_t)(uznum & (size_t) CARL_MASK_SCOFFT);

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#  pragma warning(pop)
#endif
}

/*
** unsigned size_t to signed int
*/

int carlx_uztosi(size_t uznum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(uznum <= (size_t) CARL_MASK_SINT);
  return (int)(uznum & (size_t) CARL_MASK_SINT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** unsigned size_t to unsigned long
*/

unsigned long carlx_uztoul(size_t uznum)
{
#ifdef __INTEL_COMPILER
# pragma warning(push)
# pragma warning(disable:810) /* conversion may lose significant bits */
#endif

#if (SIZEOF_LONG < SIZEOF_SIZE_T)
  DEBUGASSERT(uznum <= (size_t) CARL_MASK_ULONG);
#endif
  return (unsigned long)(uznum & (size_t) CARL_MASK_ULONG);

#ifdef __INTEL_COMPILER
# pragma warning(pop)
#endif
}

/*
** unsigned size_t to unsigned int
*/

unsigned int carlx_uztoui(size_t uznum)
{
#ifdef __INTEL_COMPILER
# pragma warning(push)
# pragma warning(disable:810) /* conversion may lose significant bits */
#endif

#if (SIZEOF_INT < SIZEOF_SIZE_T)
  DEBUGASSERT(uznum <= (size_t) CARL_MASK_UINT);
#endif
  return (unsigned int)(uznum & (size_t) CARL_MASK_UINT);

#ifdef __INTEL_COMPILER
# pragma warning(pop)
#endif
}

/*
** signed long to signed int
*/

int carlx_sltosi(long slnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(slnum >= 0);
#if (SIZEOF_INT < SIZEOF_LONG)
  DEBUGASSERT((unsigned long) slnum <= (unsigned long) CARL_MASK_SINT);
#endif
  return (int)(slnum & (long) CARL_MASK_SINT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** signed long to unsigned int
*/

unsigned int carlx_sltoui(long slnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(slnum >= 0);
#if (SIZEOF_INT < SIZEOF_LONG)
  DEBUGASSERT((unsigned long) slnum <= (unsigned long) CARL_MASK_UINT);
#endif
  return (unsigned int)(slnum & (long) CARL_MASK_UINT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** signed long to unsigned short
*/

unsigned short carlx_sltous(long slnum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(slnum >= 0);
  DEBUGASSERT((unsigned long) slnum <= (unsigned long) CARL_MASK_USHORT);
  return (unsigned short)(slnum & (long) CARL_MASK_USHORT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** unsigned size_t to signed ssize_t
*/

ssize_t carlx_uztosz(size_t uznum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(uznum <= (size_t) CARL_MASK_SSIZE_T);
  return (ssize_t)(uznum & (size_t) CARL_MASK_SSIZE_T);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** signed carl_off_t to unsigned size_t
*/

size_t carlx_sotouz(carl_off_t sonum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(sonum >= 0);
  return (size_t)(sonum & (carl_off_t) CARL_MASK_USIZE_T);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** signed ssize_t to signed int
*/

int carlx_sztosi(ssize_t sznum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(sznum >= 0);
#if (SIZEOF_INT < SIZEOF_SIZE_T)
  DEBUGASSERT((size_t) sznum <= (size_t) CARL_MASK_SINT);
#endif
  return (int)(sznum & (ssize_t) CARL_MASK_SINT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** unsigned int to unsigned short
*/

unsigned short carlx_uitous(unsigned int uinum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(uinum <= (unsigned int) CARL_MASK_USHORT);
  return (unsigned short) (uinum & (unsigned int) CARL_MASK_USHORT);

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

/*
** signed int to unsigned size_t
*/

size_t carlx_sitouz(int sinum)
{
#ifdef __INTEL_COMPILER
#  pragma warning(push)
#  pragma warning(disable:810) /* conversion may lose significant bits */
#endif

  DEBUGASSERT(sinum >= 0);
  return (size_t) sinum;

#ifdef __INTEL_COMPILER
#  pragma warning(pop)
#endif
}

#ifdef USE_WINSOCK

/*
** carl_socket_t to signed int
*/

int carlx_sktosi(carl_socket_t s)
{
  return (int)((ssize_t) s);
}

/*
** signed int to carl_socket_t
*/

carl_socket_t carlx_sitosk(int i)
{
  return (carl_socket_t)((ssize_t) i);
}

#endif /* USE_WINSOCK */

#if defined(WIN32) || defined(_WIN32)

ssize_t carlx_read(int fd, void *buf, size_t count)
{
  return (ssize_t)read(fd, buf, carlx_uztoui(count));
}

ssize_t carlx_write(int fd, const void *buf, size_t count)
{
  return (ssize_t)write(fd, buf, carlx_uztoui(count));
}

#endif /* WIN32 || _WIN32 */

#if defined(__INTEL_COMPILER) && defined(__unix__)

int carlx_FD_ISSET(int fd, fd_set *fdset)
{
  #pragma warning(push)
  #pragma warning(disable:1469) /* clobber ignored */
  return FD_ISSET(fd, fdset);
  #pragma warning(pop)
}

void carlx_FD_SET(int fd, fd_set *fdset)
{
  #pragma warning(push)
  #pragma warning(disable:1469) /* clobber ignored */
  FD_SET(fd, fdset);
  #pragma warning(pop)
}

void carlx_FD_ZERO(fd_set *fdset)
{
  #pragma warning(push)
  #pragma warning(disable:593) /* variable was set but never used */
  FD_ZERO(fdset);
  #pragma warning(pop)
}

unsigned short carlx_htons(unsigned short usnum)
{
#if (__INTEL_COMPILER == 910) && defined(__i386__)
  return (unsigned short)(((usnum << 8) & 0xFF00) | ((usnum >> 8) & 0x00FF));
#else
  #pragma warning(push)
  #pragma warning(disable:810) /* conversion may lose significant bits */
  return htons(usnum);
  #pragma warning(pop)
#endif
}

unsigned short carlx_ntohs(unsigned short usnum)
{
#if (__INTEL_COMPILER == 910) && defined(__i386__)
  return (unsigned short)(((usnum << 8) & 0xFF00) | ((usnum >> 8) & 0x00FF));
#else
  #pragma warning(push)
  #pragma warning(disable:810) /* conversion may lose significant bits */
  return ntohs(usnum);
  #pragma warning(pop)
#endif
}

#endif /* __INTEL_COMPILER && __unix__ */
