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
 * An example of carl_easy_send() and carl_easy_recv() usage.
 * </DESC>
 */

#include <stdio.h>
#include <string.h>
#include <carl/carl.h>

/* Auxiliary function that waits on the socket. */
static int wait_on_socket(carl_socket_t sockfd, int for_recv, long timeout_ms)
{
  struct timeval tv;
  fd_set infd, outfd, errfd;
  int res;

  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  FD_ZERO(&infd);
  FD_ZERO(&outfd);
  FD_ZERO(&errfd);

  FD_SET(sockfd, &errfd); /* always check for error */

  if(for_recv) {
    FD_SET(sockfd, &infd);
  }
  else {
    FD_SET(sockfd, &outfd);
  }

  /* select() returns the number of signalled sockets or -1 */
  res = select((int)sockfd + 1, &infd, &outfd, &errfd, &tv);
  return res;
}

int main(void)
{
  CARL *carl;
  /* Minimalistic http request */
  const char *request = "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n";
  size_t request_len = strlen(request);

  /* A general note of caution here: if you're using carl_easy_recv() or
     carl_easy_send() to implement HTTP or _any_ other protocol libcarl
     supports "natively", you're doing it wrong and you should stop.

     This example uses HTTP only to show how to use this API, it does not
     suggest that writing an application doing this is sensible.
  */

  carl = carl_easy_init();
  if(carl) {
    CARLcode res;
    carl_socket_t sockfd;
    size_t nsent_total = 0;

    carl_easy_setopt(carl, CARLOPT_URL, "https://example.com");
    /* Do not do the transfer - only connect to host */
    carl_easy_setopt(carl, CARLOPT_CONNECT_ONLY, 1L);
    res = carl_easy_perform(carl);

    if(res != CARLE_OK) {
      printf("Error: %s\n", carl_easy_strerror(res));
      return 1;
    }

    /* Extract the socket from the carl handle - we'll need it for waiting. */
    res = carl_easy_getinfo(carl, CARLINFO_ACTIVESOCKET, &sockfd);

    if(res != CARLE_OK) {
      printf("Error: %s\n", carl_easy_strerror(res));
      return 1;
    }

    printf("Sending request.\n");

    do {
      /* Warning: This example program may loop indefinitely.
       * A production-quality program must define a timeout and exit this loop
       * as soon as the timeout has expired. */
      size_t nsent;
      do {
        nsent = 0;
        res = carl_easy_send(carl, request + nsent_total,
            request_len - nsent_total, &nsent);
        nsent_total += nsent;

        if(res == CARLE_AGAIN && !wait_on_socket(sockfd, 0, 60000L)) {
          printf("Error: timeout.\n");
          return 1;
        }
      } while(res == CARLE_AGAIN);

      if(res != CARLE_OK) {
        printf("Error: %s\n", carl_easy_strerror(res));
        return 1;
      }

      printf("Sent %" CARL_FORMAT_CARL_OFF_T " bytes.\n",
        (carl_off_t)nsent);

    } while(nsent_total < request_len);

    printf("Reading response.\n");

    for(;;) {
      /* Warning: This example program may loop indefinitely (see above). */
      char buf[1024];
      size_t nread;
      do {
        nread = 0;
        res = carl_easy_recv(carl, buf, sizeof(buf), &nread);

        if(res == CARLE_AGAIN && !wait_on_socket(sockfd, 1, 60000L)) {
          printf("Error: timeout.\n");
          return 1;
        }
      } while(res == CARLE_AGAIN);

      if(res != CARLE_OK) {
        printf("Error: %s\n", carl_easy_strerror(res));
        break;
      }

      if(nread == 0) {
        /* end of the response */
        break;
      }

      printf("Received %" CARL_FORMAT_CARL_OFF_T " bytes.\n",
        (carl_off_t)nread);
    }

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  return 0;
}
