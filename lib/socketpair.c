/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2019 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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
#include "socketpair.h"

#if !defined(HAVE_SOCKETPAIR) && !defined(CARL_DISABLE_SOCKETPAIR)
#ifdef WIN32
/*
 * This is a socketpair() implementation for Windows.
 */
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#else
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h> /* IPPROTO_TCP */
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001
#endif /* !INADDR_LOOPBACK */
#endif /* !WIN32 */

/* The last 3 #include files should be in this order */
#include "carl_printf.h"
#include "carl_memory.h"
#include "memdebug.h"

int Curl_socketpair(int domain, int type, int protocol,
                    carl_socket_t socks[2])
{
  union {
    struct sockaddr_in inaddr;
    struct sockaddr addr;
  } a;
  carl_socket_t listener;
  carl_socklen_t addrlen = sizeof(a.inaddr);
  int reuse = 1;
  char data[2][12];
  ssize_t dlen;
  (void)domain;
  (void)type;
  (void)protocol;

  listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(listener == CARL_SOCKET_BAD)
    return -1;

  memset(&a, 0, sizeof(a));
  a.inaddr.sin_family = AF_INET;
  a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  a.inaddr.sin_port = 0;

  socks[0] = socks[1] = CARL_SOCKET_BAD;

  if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
                (char *)&reuse, (carl_socklen_t)sizeof(reuse)) == -1)
    goto error;
  if(bind(listener, &a.addr, sizeof(a.inaddr)) == -1)
    goto error;
  if(getsockname(listener, &a.addr, &addrlen) == -1)
    goto error;
  if(listen(listener, 1) == -1)
    goto error;
  socks[0] = socket(AF_INET, SOCK_STREAM, 0);
  if(socks[0] == CARL_SOCKET_BAD)
    goto error;
  if(connect(socks[0], &a.addr, sizeof(a.inaddr)) == -1)
    goto error;
  socks[1] = accept(listener, NULL, NULL);
  if(socks[1] == CARL_SOCKET_BAD)
    goto error;

  /* verify that nothing else connected */
  msnprintf(data[0], sizeof(data[0]), "%p", socks);
  dlen = strlen(data[0]);
  if(swrite(socks[0], data[0], dlen) != dlen)
    goto error;
  if(sread(socks[1], data[1], sizeof(data[1])) != dlen)
    goto error;
  if(memcmp(data[0], data[1], dlen))
    goto error;

  sclose(listener);
  return 0;

  error:
  sclose(listener);
  sclose(socks[0]);
  sclose(socks[1]);
  return -1;
}

#endif /* ! HAVE_SOCKETPAIR */
