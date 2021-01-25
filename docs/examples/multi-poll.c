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
 * single download with the multi interface's carl_multi_poll
 * </DESC>
 */

#include <stdio.h>
#include <string.h>

/* somewhat unix-specific */
#include <sys/time.h>
#include <unistd.h>

/* carl stuff */
#include <carl/carl.h>

int main(void)
{
  CARL *http_handle;
  CARLM *multi_handle;
  int still_running = 1; /* keep number of running handles */

  carl_global_init(CARL_GLOBAL_DEFAULT);

  http_handle = carl_easy_init();

  carl_easy_setopt(http_handle, CARLOPT_URL, "https://www.example.com/");

  multi_handle = carl_multi_init();

  carl_multi_add_handle(multi_handle, http_handle);

  while(still_running) {
    CARLMcode mc; /* carl_multi_poll() return code */
    int numfds;

    /* we start some action by calling perform right away */
    mc = carl_multi_perform(multi_handle, &still_running);

    if(still_running)
      /* wait for activity, timeout or "nothing" */
      mc = carl_multi_poll(multi_handle, NULL, 0, 1000, &numfds);

    if(mc != CARLM_OK) {
      fprintf(stderr, "carl_multi_wait() failed, code %d.\n", mc);
      break;
    }
  }

  carl_multi_remove_handle(multi_handle, http_handle);
  carl_easy_cleanup(http_handle);
  carl_multi_cleanup(multi_handle);
  carl_global_cleanup();

  return 0;
}
