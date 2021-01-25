#ifndef HEADER_CARL_MEMDEBUG_H
#define HEADER_CARL_MEMDEBUG_H
#ifdef CARLDEBUG
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
 * CAUTION: this header is designed to work when included by the app-side
 * as well as the library. Do not mix with library internals!
 */

#define CARL_MT_LOGFNAME_BUFSIZE 512

extern FILE *carl_dbg_logfile;

/* memory functions */
CARL_EXTERN void *carl_dbg_malloc(size_t size, int line, const char *source);
CARL_EXTERN void *carl_dbg_calloc(size_t elements, size_t size, int line,
                                  const char *source);
CARL_EXTERN void *carl_dbg_realloc(void *ptr, size_t size, int line,
                                   const char *source);
CARL_EXTERN void carl_dbg_free(void *ptr, int line, const char *source);
CARL_EXTERN char *carl_dbg_strdup(const char *str, int line, const char *src);
#if defined(WIN32) && defined(UNICODE)
CARL_EXTERN wchar_t *carl_dbg_wcsdup(const wchar_t *str, int line,
                                     const char *source);
#endif

CARL_EXTERN void carl_dbg_memdebug(const char *logname);
CARL_EXTERN void carl_dbg_memlimit(long limit);
CARL_EXTERN void carl_dbg_log(const char *format, ...);

/* file descriptor manipulators */
CARL_EXTERN carl_socket_t carl_dbg_socket(int domain, int type, int protocol,
                                          int line, const char *source);
CARL_EXTERN void carl_dbg_mark_sclose(carl_socket_t sockfd,
                                      int line, const char *source);
CARL_EXTERN int carl_dbg_sclose(carl_socket_t sockfd,
                                int line, const char *source);
CARL_EXTERN carl_socket_t carl_dbg_accept(carl_socket_t s, void *a, void *alen,
                                          int line, const char *source);
#ifdef HAVE_SOCKETPAIR
CARL_EXTERN int carl_dbg_socketpair(int domain, int type, int protocol,
                                    carl_socket_t socket_vector[2],
                                    int line, const char *source);
#endif

/* send/receive sockets */
CARL_EXTERN SEND_TYPE_RETV carl_dbg_send(SEND_TYPE_ARG1 sockfd,
                                         SEND_QUAL_ARG2 SEND_TYPE_ARG2 buf,
                                         SEND_TYPE_ARG3 len,
                                         SEND_TYPE_ARG4 flags, int line,
                                         const char *source);
CARL_EXTERN RECV_TYPE_RETV carl_dbg_recv(RECV_TYPE_ARG1 sockfd,
                                         RECV_TYPE_ARG2 buf,
                                         RECV_TYPE_ARG3 len,
                                         RECV_TYPE_ARG4 flags, int line,
                                         const char *source);

/* FILE functions */
CARL_EXTERN FILE *carl_dbg_fopen(const char *file, const char *mode, int line,
                                 const char *source);
CARL_EXTERN FILE *carl_dbg_fdopen(int filedes, const char *mode,
                                  int line, const char *source);

CARL_EXTERN int carl_dbg_fclose(FILE *file, int line, const char *source);

#ifndef MEMDEBUG_NODEFINES

/* Set this symbol on the command-line, recompile all lib-sources */
#undef strdup
#define strdup(ptr) carl_dbg_strdup(ptr, __LINE__, __FILE__)
#define malloc(size) carl_dbg_malloc(size, __LINE__, __FILE__)
#define calloc(nbelem,size) carl_dbg_calloc(nbelem, size, __LINE__, __FILE__)
#define realloc(ptr,size) carl_dbg_realloc(ptr, size, __LINE__, __FILE__)
#define free(ptr) carl_dbg_free(ptr, __LINE__, __FILE__)
#define send(a,b,c,d) carl_dbg_send(a,b,c,d, __LINE__, __FILE__)
#define recv(a,b,c,d) carl_dbg_recv(a,b,c,d, __LINE__, __FILE__)

#ifdef WIN32
#  ifdef UNICODE
#    undef wcsdup
#    define wcsdup(ptr) carl_dbg_wcsdup(ptr, __LINE__, __FILE__)
#    undef _wcsdup
#    define _wcsdup(ptr) carl_dbg_wcsdup(ptr, __LINE__, __FILE__)
#    undef _tcsdup
#    define _tcsdup(ptr) carl_dbg_wcsdup(ptr, __LINE__, __FILE__)
#  else
#    undef _tcsdup
#    define _tcsdup(ptr) carl_dbg_strdup(ptr, __LINE__, __FILE__)
#  endif
#endif

#undef socket
#define socket(domain,type,protocol)\
 carl_dbg_socket(domain, type, protocol, __LINE__, __FILE__)
#undef accept /* for those with accept as a macro */
#define accept(sock,addr,len)\
 carl_dbg_accept(sock, addr, len, __LINE__, __FILE__)
#ifdef HAVE_SOCKETPAIR
#define socketpair(domain,type,protocol,socket_vector)\
 carl_dbg_socketpair(domain, type, protocol, socket_vector, __LINE__, __FILE__)
#endif

#ifdef HAVE_GETADDRINFO
#if defined(getaddrinfo) && defined(__osf__)
/* OSF/1 and Tru64 have getaddrinfo as a define already, so we cannot define
   our macro as for other platforms. Instead, we redefine the new name they
   define getaddrinfo to become! */
#define ogetaddrinfo(host,serv,hint,res) \
  carl_dbg_getaddrinfo(host, serv, hint, res, __LINE__, __FILE__)
#else
#undef getaddrinfo
#define getaddrinfo(host,serv,hint,res) \
  carl_dbg_getaddrinfo(host, serv, hint, res, __LINE__, __FILE__)
#endif
#endif /* HAVE_GETADDRINFO */

#ifdef HAVE_FREEADDRINFO
#undef freeaddrinfo
#define freeaddrinfo(data) \
  carl_dbg_freeaddrinfo(data, __LINE__, __FILE__)
#endif /* HAVE_FREEADDRINFO */

/* sclose is probably already defined, redefine it! */
#undef sclose
#define sclose(sockfd) carl_dbg_sclose(sockfd,__LINE__,__FILE__)

#define fake_sclose(sockfd) carl_dbg_mark_sclose(sockfd,__LINE__,__FILE__)

#undef fopen
#define fopen(file,mode) carl_dbg_fopen(file,mode,__LINE__,__FILE__)
#undef fdopen
#define fdopen(file,mode) carl_dbg_fdopen(file,mode,__LINE__,__FILE__)
#define fclose(file) carl_dbg_fclose(file,__LINE__,__FILE__)

#endif /* MEMDEBUG_NODEFINES */

#endif /* CARLDEBUG */

/*
** Following section applies even when CARLDEBUG is not defined.
*/

#ifndef fake_sclose
#define fake_sclose(x)  Curl_nop_stmt
#endif

/*
 * Curl_safefree defined as a macro to allow MemoryTracking feature
 * to log free() calls at same location where Curl_safefree is used.
 * This macro also assigns NULL to given pointer when free'd.
 */

#define Curl_safefree(ptr) \
  do { free((ptr)); (ptr) = NULL;} while(0)

#endif /* HEADER_CARL_MEMDEBUG_H */
