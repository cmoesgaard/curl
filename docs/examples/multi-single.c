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
 * using the multi interface to do a single download
 * </DESC>
 */

#include <stdio.h>
#include <string.h>

/* somewhat unix-specific */
#include <sys/time.h>
#include <unistd.h>

/* carl stuff */
#include <carl/carl.h>

#ifdef _WIN32
#define WAITMS(x) Sleep(x)
#else
/* Portable sleep for platforms other than Windows. */
#define WAITMS(x)                               \
  struct timeval wait = { 0, (x) * 1000 };      \
  (void)select(0, NULL, NULL, NULL, &wait)
#endif

/*
 * Simply download a HTTP file.
 */
int main(void)
{
  CARL *http_handle;
  CARLM *multi_handle;

  int still_running = 0; /* keep number of running handles */
  int repeats = 0;

  carl_global_init(CARL_GLOBAL_DEFAULT);

  http_handle = carl_easy_init();

  /* set the options (I left out a few, you'll get the point anyway) */
  carl_easy_setopt(http_handle, CARLOPT_URL, "https://www.example.com/");

  /* init a multi stack */
  multi_handle = carl_multi_init();

  /* add the individual transfers */
  carl_multi_add_handle(multi_handle, http_handle);

  /* we start some action by calling perform right away */
  carl_multi_perform(multi_handle, &still_running);

  while(still_running) {
    CARLMcode mc; /* carl_multi_wait() return code */
    int numfds;

    /* wait for activity, timeout or "nothing" */
    mc = carl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);

    if(mc != CARLM_OK) {
      fprintf(stderr, "carl_multi_wait() failed, code %d.\n", mc);
      break;
    }

    /* 'numfds' being zero means either a timeout or no file descriptors to
       wait for. Try timeout on first occurrence, then assume no file
       descriptors and no file descriptors to wait for means wait for 100
       milliseconds. */

    if(!numfds) {
      repeats++; /* count number of repeated zero numfds */
      if(repeats > 1) {
        WAITMS(100); /* sleep 100 milliseconds */
      }
    }
    else
      repeats = 0;

    carl_multi_perform(multi_handle, &still_running);
  }

  carl_multi_remove_handle(multi_handle, http_handle);

  carl_easy_cleanup(http_handle);

  carl_multi_cleanup(multi_handle);

  carl_global_cleanup();

  return 0;
}
