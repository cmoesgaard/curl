#ifndef CARLINC_SYSTEM_H
#define CARLINC_SYSTEM_H
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

/*
 * Try to keep one section per platform, compiler and architecture, otherwise,
 * if an existing section is reused for a different one and later on the
 * original is adjusted, probably the piggybacking one can be adversely
 * changed.
 *
 * In order to differentiate between platforms/compilers/architectures use
 * only compiler built in predefined preprocessor symbols.
 *
 * carl_off_t
 * ----------
 *
 * For any given platform/compiler carl_off_t must be typedef'ed to a 64-bit
 * wide signed integral data type. The width of this data type must remain
 * constant and independent of any possible large file support settings.
 *
 * As an exception to the above, carl_off_t shall be typedef'ed to a 32-bit
 * wide signed integral data type if there is no 64-bit type.
 *
 * As a general rule, carl_off_t shall not be mapped to off_t. This rule shall
 * only be violated if off_t is the only 64-bit data type available and the
 * size of off_t is independent of large file support settings. Keep your
 * build on the safe side avoiding an off_t gating.  If you have a 64-bit
 * off_t then take for sure that another 64-bit data type exists, dig deeper
 * and you will find it.
 *
 */

#if defined(__DJGPP__) || defined(__GO32__)
#  if defined(__DJGPP__) && (__DJGPP__ > 1)
#    define CARL_TYPEOF_CARL_OFF_T     long long
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#  else
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

#elif defined(__SALFORDC__)
#  define CARL_TYPEOF_CARL_OFF_T     long
#  define CARL_FORMAT_CARL_OFF_T     "ld"
#  define CARL_FORMAT_CARL_OFF_TU    "lu"
#  define CARL_SUFFIX_CARL_OFF_T     L
#  define CARL_SUFFIX_CARL_OFF_TU    UL
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

#elif defined(__BORLANDC__)
#  if (__BORLANDC__ < 0x520)
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#  else
#    define CARL_TYPEOF_CARL_OFF_T     __int64
#    define CARL_FORMAT_CARL_OFF_T     "I64d"
#    define CARL_FORMAT_CARL_OFF_TU    "I64u"
#    define CARL_SUFFIX_CARL_OFF_T     i64
#    define CARL_SUFFIX_CARL_OFF_TU    ui64
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

#elif defined(__TURBOC__)
#  define CARL_TYPEOF_CARL_OFF_T     long
#  define CARL_FORMAT_CARL_OFF_T     "ld"
#  define CARL_FORMAT_CARL_OFF_TU    "lu"
#  define CARL_SUFFIX_CARL_OFF_T     L
#  define CARL_SUFFIX_CARL_OFF_TU    UL
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

#elif defined(__WATCOMC__)
#  if defined(__386__)
#    define CARL_TYPEOF_CARL_OFF_T     __int64
#    define CARL_FORMAT_CARL_OFF_T     "I64d"
#    define CARL_FORMAT_CARL_OFF_TU    "I64u"
#    define CARL_SUFFIX_CARL_OFF_T     i64
#    define CARL_SUFFIX_CARL_OFF_TU    ui64
#  else
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

#elif defined(__POCC__)
#  if (__POCC__ < 280)
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#  elif defined(_MSC_VER)
#    define CARL_TYPEOF_CARL_OFF_T     __int64
#    define CARL_FORMAT_CARL_OFF_T     "I64d"
#    define CARL_FORMAT_CARL_OFF_TU    "I64u"
#    define CARL_SUFFIX_CARL_OFF_T     i64
#    define CARL_SUFFIX_CARL_OFF_TU    ui64
#  else
#    define CARL_TYPEOF_CARL_OFF_T     long long
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

#elif defined(__LCC__)
#  if defined(__e2k__) /* MCST eLbrus C Compiler */
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#    define CARL_TYPEOF_CARL_SOCKLEN_T socklen_t
#    define CARL_PULL_SYS_TYPES_H      1
#    define CARL_PULL_SYS_SOCKET_H     1
#  else                /* Local (or Little) C Compiler */
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#    define CARL_TYPEOF_CARL_SOCKLEN_T int
#  endif

#elif defined(__SYMBIAN32__)
#  if defined(__EABI__) /* Treat all ARM compilers equally */
#    define CARL_TYPEOF_CARL_OFF_T     long long
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#  elif defined(__CW32__)
#    pragma longlong on
#    define CARL_TYPEOF_CARL_OFF_T     long long
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#  elif defined(__VC32__)
#    define CARL_TYPEOF_CARL_OFF_T     __int64
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T unsigned int

#elif defined(__MWERKS__)
#  define CARL_TYPEOF_CARL_OFF_T     long long
#  define CARL_FORMAT_CARL_OFF_T     "lld"
#  define CARL_FORMAT_CARL_OFF_TU    "llu"
#  define CARL_SUFFIX_CARL_OFF_T     LL
#  define CARL_SUFFIX_CARL_OFF_TU    ULL
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

#elif defined(_WIN32_WCE)
#  define CARL_TYPEOF_CARL_OFF_T     __int64
#  define CARL_FORMAT_CARL_OFF_T     "I64d"
#  define CARL_FORMAT_CARL_OFF_TU    "I64u"
#  define CARL_SUFFIX_CARL_OFF_T     i64
#  define CARL_SUFFIX_CARL_OFF_TU    ui64
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

#elif defined(__MINGW32__)
#  define CARL_TYPEOF_CARL_OFF_T     long long
#  define CARL_FORMAT_CARL_OFF_T     "I64d"
#  define CARL_FORMAT_CARL_OFF_TU    "I64u"
#  define CARL_SUFFIX_CARL_OFF_T     LL
#  define CARL_SUFFIX_CARL_OFF_TU    ULL
#  define CARL_TYPEOF_CARL_SOCKLEN_T socklen_t
#  define CARL_PULL_SYS_TYPES_H      1
#  define CARL_PULL_WS2TCPIP_H       1

#elif defined(__VMS)
#  if defined(__VAX)
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#  else
#    define CARL_TYPEOF_CARL_OFF_T     long long
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T unsigned int

#elif defined(__OS400__)
#  if defined(__ILEC400__)
#    define CARL_TYPEOF_CARL_OFF_T     long long
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#    define CARL_TYPEOF_CARL_SOCKLEN_T socklen_t
#    define CARL_PULL_SYS_TYPES_H      1
#    define CARL_PULL_SYS_SOCKET_H     1
#  endif

#elif defined(__MVS__)
#  if defined(__IBMC__) || defined(__IBMCPP__)
#    if defined(_ILP32)
#    elif defined(_LP64)
#    endif
#    if defined(_LONG_LONG)
#      define CARL_TYPEOF_CARL_OFF_T     long long
#      define CARL_FORMAT_CARL_OFF_T     "lld"
#      define CARL_FORMAT_CARL_OFF_TU    "llu"
#      define CARL_SUFFIX_CARL_OFF_T     LL
#      define CARL_SUFFIX_CARL_OFF_TU    ULL
#    elif defined(_LP64)
#      define CARL_TYPEOF_CARL_OFF_T     long
#      define CARL_FORMAT_CARL_OFF_T     "ld"
#      define CARL_FORMAT_CARL_OFF_TU    "lu"
#      define CARL_SUFFIX_CARL_OFF_T     L
#      define CARL_SUFFIX_CARL_OFF_TU    UL
#    else
#      define CARL_TYPEOF_CARL_OFF_T     long
#      define CARL_FORMAT_CARL_OFF_T     "ld"
#      define CARL_FORMAT_CARL_OFF_TU    "lu"
#      define CARL_SUFFIX_CARL_OFF_T     L
#      define CARL_SUFFIX_CARL_OFF_TU    UL
#    endif
#    define CARL_TYPEOF_CARL_SOCKLEN_T socklen_t
#    define CARL_PULL_SYS_TYPES_H      1
#    define CARL_PULL_SYS_SOCKET_H     1
#  endif

#elif defined(__370__)
#  if defined(__IBMC__) || defined(__IBMCPP__)
#    if defined(_ILP32)
#    elif defined(_LP64)
#    endif
#    if defined(_LONG_LONG)
#      define CARL_TYPEOF_CARL_OFF_T     long long
#      define CARL_FORMAT_CARL_OFF_T     "lld"
#      define CARL_FORMAT_CARL_OFF_TU    "llu"
#      define CARL_SUFFIX_CARL_OFF_T     LL
#      define CARL_SUFFIX_CARL_OFF_TU    ULL
#    elif defined(_LP64)
#      define CARL_TYPEOF_CARL_OFF_T     long
#      define CARL_FORMAT_CARL_OFF_T     "ld"
#      define CARL_FORMAT_CARL_OFF_TU    "lu"
#      define CARL_SUFFIX_CARL_OFF_T     L
#      define CARL_SUFFIX_CARL_OFF_TU    UL
#    else
#      define CARL_TYPEOF_CARL_OFF_T     long
#      define CARL_FORMAT_CARL_OFF_T     "ld"
#      define CARL_FORMAT_CARL_OFF_TU    "lu"
#      define CARL_SUFFIX_CARL_OFF_T     L
#      define CARL_SUFFIX_CARL_OFF_TU    UL
#    endif
#    define CARL_TYPEOF_CARL_SOCKLEN_T socklen_t
#    define CARL_PULL_SYS_TYPES_H      1
#    define CARL_PULL_SYS_SOCKET_H     1
#  endif

#elif defined(TPF)
#  define CARL_TYPEOF_CARL_OFF_T     long
#  define CARL_FORMAT_CARL_OFF_T     "ld"
#  define CARL_FORMAT_CARL_OFF_TU    "lu"
#  define CARL_SUFFIX_CARL_OFF_T     L
#  define CARL_SUFFIX_CARL_OFF_TU    UL
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

#elif defined(__TINYC__) /* also known as tcc */
#  define CARL_TYPEOF_CARL_OFF_T     long long
#  define CARL_FORMAT_CARL_OFF_T     "lld"
#  define CARL_FORMAT_CARL_OFF_TU    "llu"
#  define CARL_SUFFIX_CARL_OFF_T     LL
#  define CARL_SUFFIX_CARL_OFF_TU    ULL
#  define CARL_TYPEOF_CARL_SOCKLEN_T socklen_t
#  define CARL_PULL_SYS_TYPES_H      1
#  define CARL_PULL_SYS_SOCKET_H     1

#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC) /* Oracle Solaris Studio */
#  if !defined(__LP64) && (defined(__ILP32) ||                          \
                           defined(__i386) ||                           \
                           defined(__sparcv8) ||                        \
                           defined(__sparcv8plus))
#    define CARL_TYPEOF_CARL_OFF_T     long long
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#  elif defined(__LP64) || \
        defined(__amd64) || defined(__sparcv9)
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T socklen_t
#  define CARL_PULL_SYS_TYPES_H      1
#  define CARL_PULL_SYS_SOCKET_H     1

#elif defined(__xlc__) /* IBM xlc compiler */
#  if !defined(_LP64)
#    define CARL_TYPEOF_CARL_OFF_T     long long
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#  else
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T socklen_t
#  define CARL_PULL_SYS_TYPES_H      1
#  define CARL_PULL_SYS_SOCKET_H     1

/* ===================================== */
/*    KEEP MSVC THE PENULTIMATE ENTRY    */
/* ===================================== */

#elif defined(_MSC_VER)
#  if (_MSC_VER >= 900) && (_INTEGRAL_MAX_BITS >= 64)
#    define CARL_TYPEOF_CARL_OFF_T     __int64
#    define CARL_FORMAT_CARL_OFF_T     "I64d"
#    define CARL_FORMAT_CARL_OFF_TU    "I64u"
#    define CARL_SUFFIX_CARL_OFF_T     i64
#    define CARL_SUFFIX_CARL_OFF_TU    ui64
#  else
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T int

/* ===================================== */
/*    KEEP GENERIC GCC THE LAST ENTRY    */
/* ===================================== */

#elif defined(__GNUC__) && !defined(_SCO_DS)
#  if !defined(__LP64__) &&                                             \
  (defined(__ILP32__) || defined(__i386__) || defined(__hppa__) ||      \
   defined(__ppc__) || defined(__powerpc__) || defined(__arm__) ||      \
   defined(__sparc__) || defined(__mips__) || defined(__sh__) ||        \
   defined(__XTENSA__) ||                                               \
   (defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 4)  ||               \
   (defined(__LONG_MAX__) && __LONG_MAX__ == 2147483647L))
#    define CARL_TYPEOF_CARL_OFF_T     long long
#    define CARL_FORMAT_CARL_OFF_T     "lld"
#    define CARL_FORMAT_CARL_OFF_TU    "llu"
#    define CARL_SUFFIX_CARL_OFF_T     LL
#    define CARL_SUFFIX_CARL_OFF_TU    ULL
#  elif defined(__LP64__) || \
        defined(__x86_64__) || defined(__ppc64__) || defined(__sparc64__) || \
        defined(__e2k__) || \
        (defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 8) || \
        (defined(__LONG_MAX__) && __LONG_MAX__ == 9223372036854775807L)
#    define CARL_TYPEOF_CARL_OFF_T     long
#    define CARL_FORMAT_CARL_OFF_T     "ld"
#    define CARL_FORMAT_CARL_OFF_TU    "lu"
#    define CARL_SUFFIX_CARL_OFF_T     L
#    define CARL_SUFFIX_CARL_OFF_TU    UL
#  endif
#  define CARL_TYPEOF_CARL_SOCKLEN_T socklen_t
#  define CARL_PULL_SYS_TYPES_H      1
#  define CARL_PULL_SYS_SOCKET_H     1

#else
/* generic "safe guess" on old 32 bit style */
# define CARL_TYPEOF_CARL_OFF_T     long
# define CARL_FORMAT_CARL_OFF_T     "ld"
# define CARL_FORMAT_CARL_OFF_TU    "lu"
# define CARL_SUFFIX_CARL_OFF_T     L
# define CARL_SUFFIX_CARL_OFF_TU    UL
# define CARL_TYPEOF_CARL_SOCKLEN_T int
#endif

#ifdef _AIX
/* AIX needs <sys/poll.h> */
#define CARL_PULL_SYS_POLL_H
#endif


/* CARL_PULL_WS2TCPIP_H is defined above when inclusion of header file  */
/* ws2tcpip.h is required here to properly make type definitions below. */
#ifdef CARL_PULL_WS2TCPIP_H
#  include <winsock2.h>
#  include <windows.h>
#  include <ws2tcpip.h>
#endif

/* CARL_PULL_SYS_TYPES_H is defined above when inclusion of header file  */
/* sys/types.h is required here to properly make type definitions below. */
#ifdef CARL_PULL_SYS_TYPES_H
#  include <sys/types.h>
#endif

/* CARL_PULL_SYS_SOCKET_H is defined above when inclusion of header file  */
/* sys/socket.h is required here to properly make type definitions below. */
#ifdef CARL_PULL_SYS_SOCKET_H
#  include <sys/socket.h>
#endif

/* CARL_PULL_SYS_POLL_H is defined above when inclusion of header file    */
/* sys/poll.h is required here to properly make type definitions below.   */
#ifdef CARL_PULL_SYS_POLL_H
#  include <sys/poll.h>
#endif

/* Data type definition of carl_socklen_t. */
#ifdef CARL_TYPEOF_CARL_SOCKLEN_T
  typedef CARL_TYPEOF_CARL_SOCKLEN_T carl_socklen_t;
#endif

/* Data type definition of carl_off_t. */

#ifdef CARL_TYPEOF_CARL_OFF_T
  typedef CARL_TYPEOF_CARL_OFF_T carl_off_t;
#endif

/*
 * CARL_ISOCPP and CARL_OFF_T_C definitions are done here in order to allow
 * these to be visible and exported by the external libcarl interface API,
 * while also making them visible to the library internals, simply including
 * carl_setup.h, without actually needing to include carl.h internally.
 * If some day this section would grow big enough, all this should be moved
 * to its own header file.
 */

/*
 * Figure out if we can use the ## preprocessor operator, which is supported
 * by ISO/ANSI C and C++. Some compilers support it without setting __STDC__
 * or  __cplusplus so we need to carefully check for them too.
 */

#if defined(__STDC__) || defined(_MSC_VER) || defined(__cplusplus) || \
  defined(__HP_aCC) || defined(__BORLANDC__) || defined(__LCC__) || \
  defined(__POCC__) || defined(__SALFORDC__) || defined(__HIGHC__) || \
  defined(__ILEC400__)
  /* This compiler is believed to have an ISO compatible preprocessor */
#define CARL_ISOCPP
#else
  /* This compiler is believed NOT to have an ISO compatible preprocessor */
#undef CARL_ISOCPP
#endif

/*
 * Macros for minimum-width signed and unsigned carl_off_t integer constants.
 */

#if defined(__BORLANDC__) && (__BORLANDC__ == 0x0551)
#  define CARLINC_OFF_T_C_HLPR2(x) x
#  define CARLINC_OFF_T_C_HLPR1(x) CARLINC_OFF_T_C_HLPR2(x)
#  define CARL_OFF_T_C(Val)  CARLINC_OFF_T_C_HLPR1(Val) ## \
                             CARLINC_OFF_T_C_HLPR1(CARL_SUFFIX_CARL_OFF_T)
#  define CARL_OFF_TU_C(Val) CARLINC_OFF_T_C_HLPR1(Val) ## \
                             CARLINC_OFF_T_C_HLPR1(CARL_SUFFIX_CARL_OFF_TU)
#else
#  ifdef CARL_ISOCPP
#    define CARLINC_OFF_T_C_HLPR2(Val,Suffix) Val ## Suffix
#  else
#    define CARLINC_OFF_T_C_HLPR2(Val,Suffix) Val/**/Suffix
#  endif
#  define CARLINC_OFF_T_C_HLPR1(Val,Suffix) CARLINC_OFF_T_C_HLPR2(Val,Suffix)
#  define CARL_OFF_T_C(Val)  CARLINC_OFF_T_C_HLPR1(Val,CARL_SUFFIX_CARL_OFF_T)
#  define CARL_OFF_TU_C(Val) CARLINC_OFF_T_C_HLPR1(Val,CARL_SUFFIX_CARL_OFF_TU)
#endif

#endif /* CARLINC_SYSTEM_H */
