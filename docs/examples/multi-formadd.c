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
 * using the multi interface to do a multipart formpost without blocking
 * </DESC>
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <carl/carl.h>

int main(void)
{
  CARL *carl;

  CARLM *multi_handle;
  int still_running = 0;

  struct carl_httppost *formpost = NULL;
  struct carl_httppost *lastptr = NULL;
  struct carl_slist *headerlist = NULL;
  static const char buf[] = "Expect:";

  /* Fill in the file upload field. This makes libcarl load data from
     the given file name when carl_easy_perform() is called. */
  carl_formadd(&formpost,
               &lastptr,
               CARLFORM_COPYNAME, "sendfile",
               CARLFORM_FILE, "postit2.c",
               CARLFORM_END);

  /* Fill in the filename field */
  carl_formadd(&formpost,
               &lastptr,
               CARLFORM_COPYNAME, "filename",
               CARLFORM_COPYCONTENTS, "postit2.c",
               CARLFORM_END);

  /* Fill in the submit field too, even if this is rarely needed */
  carl_formadd(&formpost,
               &lastptr,
               CARLFORM_COPYNAME, "submit",
               CARLFORM_COPYCONTENTS, "send",
               CARLFORM_END);

  carl = carl_easy_init();
  multi_handle = carl_multi_init();

  /* initialize custom header list (stating that Expect: 100-continue is not
     wanted */
  headerlist = carl_slist_append(headerlist, buf);
  if(carl && multi_handle) {

    /* what URL that receives this POST */
    carl_easy_setopt(carl, CARLOPT_URL, "https://www.example.com/upload.cgi");
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);

    carl_easy_setopt(carl, CARLOPT_HTTPHEADER, headerlist);
    carl_easy_setopt(carl, CARLOPT_HTTPPOST, formpost);

    carl_multi_add_handle(multi_handle, carl);

    carl_multi_perform(multi_handle, &still_running);

    while(still_running) {
      struct timeval timeout;
      int rc; /* select() return code */
      CARLMcode mc; /* carl_multi_fdset() return code */

      fd_set fdread;
      fd_set fdwrite;
      fd_set fdexcep;
      int maxfd = -1;

      long carl_timeo = -1;

      FD_ZERO(&fdread);
      FD_ZERO(&fdwrite);
      FD_ZERO(&fdexcep);

      /* set a suitable timeout to play around with */
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;

      carl_multi_timeout(multi_handle, &carl_timeo);
      if(carl_timeo >= 0) {
        timeout.tv_sec = carl_timeo / 1000;
        if(timeout.tv_sec > 1)
          timeout.tv_sec = 1;
        else
          timeout.tv_usec = (carl_timeo % 1000) * 1000;
      }

      /* get file descriptors from the transfers */
      mc = carl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

      if(mc != CARLM_OK) {
        fprintf(stderr, "carl_multi_fdset() failed, code %d.\n", mc);
        break;
      }

      /* On success the value of maxfd is guaranteed to be >= -1. We call
         select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
         no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
         to sleep 100ms, which is the minimum suggested value in the
         carl_multi_fdset() doc. */

      if(maxfd == -1) {
#ifdef _WIN32
        Sleep(100);
        rc = 0;
#else
        /* Portable sleep for platforms other than Windows. */
        struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
        rc = select(0, NULL, NULL, NULL, &wait);
#endif
      }
      else {
        /* Note that on some platforms 'timeout' may be modified by select().
           If you need access to the original value save a copy beforehand. */
        rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
      }

      switch(rc) {
      case -1:
        /* select error */
        break;
      case 0:
      default:
        /* timeout or readable/writable sockets */
        printf("perform!\n");
        carl_multi_perform(multi_handle, &still_running);
        printf("running: %d!\n", still_running);
        break;
      }
    }

    carl_multi_cleanup(multi_handle);

    /* always cleanup */
    carl_easy_cleanup(carl);

    /* then cleanup the formpost chain */
    carl_formfree(formpost);

    /* free slist */
    carl_slist_free_all(headerlist);
  }
  return 0;
}
