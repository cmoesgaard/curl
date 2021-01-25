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
/* <DESC>
 * An example demonstrating how an application can pass in a custom
 * socket to libcarl to use. This example also handles the connect itself.
 * </DESC>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <carl/carl.h>

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#else
#include <sys/types.h>        /*  socket types              */
#include <sys/socket.h>       /*  socket definitions        */
#include <netinet/in.h>
#include <arpa/inet.h>        /*  inet (3) functions         */
#include <unistd.h>           /*  misc. Unix functions      */
#endif

#include <errno.h>

/* The IP address and port number to connect to */
#define IPADDR "127.0.0.1"
#define PORTNUM 80

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

static int closecb(void *clientp, carl_socket_t item)
{
  (void)clientp;
  printf("libcarl wants to close %d now\n", (int)item);
  return 0;
}

static carl_socket_t opensocket(void *clientp,
                                carlsocktype purpose,
                                struct carl_sockaddr *address)
{
  carl_socket_t sockfd;
  (void)purpose;
  (void)address;
  sockfd = *(carl_socket_t *)clientp;
  /* the actual externally set socket is passed in via the OPENSOCKETDATA
     option */
  return sockfd;
}

static int sockopt_callback(void *clientp, carl_socket_t carlfd,
                            carlsocktype purpose)
{
  (void)clientp;
  (void)carlfd;
  (void)purpose;
  /* This return code was added in libcarl 7.21.5 */
  return CARL_SOCKOPT_ALREADY_CONNECTED;
}

int main(void)
{
  CARL *carl;
  CARLcode res;
  struct sockaddr_in servaddr;  /*  socket address structure  */
  carl_socket_t sockfd;

#ifdef WIN32
  WSADATA wsaData;
  int initwsa = WSAStartup(MAKEWORD(2, 0), &wsaData);
  if(initwsa != 0) {
    printf("WSAStartup failed: %d\n", initwsa);
    return 1;
  }
#endif

  carl = carl_easy_init();
  if(carl) {
    /*
     * Note that libcarl will internally think that you connect to the host
     * and port that you specify in the URL option.
     */
    carl_easy_setopt(carl, CARLOPT_URL, "http://99.99.99.99:9999");

    /* Create the socket "manually" */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == CARL_SOCKET_BAD) {
      printf("Error creating listening socket.\n");
      return 3;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(PORTNUM);

    servaddr.sin_addr.s_addr = inet_addr(IPADDR);
    if(INADDR_NONE == servaddr.sin_addr.s_addr) {
      close(sockfd);
      return 2;
    }

    if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) ==
       -1) {
      close(sockfd);
      printf("client error: connect: %s\n", strerror(errno));
      return 1;
    }

    /* no progress meter please */
    carl_easy_setopt(carl, CARLOPT_NOPROGRESS, 1L);

    /* send all data to this function  */
    carl_easy_setopt(carl, CARLOPT_WRITEFUNCTION, write_data);

    /* call this function to get a socket */
    carl_easy_setopt(carl, CARLOPT_OPENSOCKETFUNCTION, opensocket);
    carl_easy_setopt(carl, CARLOPT_OPENSOCKETDATA, &sockfd);

    /* call this function to close sockets */
    carl_easy_setopt(carl, CARLOPT_CLOSESOCKETFUNCTION, closecb);
    carl_easy_setopt(carl, CARLOPT_CLOSESOCKETDATA, &sockfd);

    /* call this function to set options for the socket */
    carl_easy_setopt(carl, CARLOPT_SOCKOPTFUNCTION, sockopt_callback);

    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1);

    res = carl_easy_perform(carl);

    carl_easy_cleanup(carl);

    close(sockfd);

    if(res) {
      printf("libcarl error: %d\n", res);
      return 4;
    }
  }

#ifdef WIN32
  WSACleanup();
#endif
  return 0;
}
