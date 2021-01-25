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
#include "test.h"
#include "memdebug.h"

static void my_lock(CARL *handle, carl_lock_data data,
                    carl_lock_access laccess, void *useptr)
{
  (void)handle;
  (void)data;
  (void)laccess;
  (void)useptr;
  printf("-> Mutex lock\n");
}

static void my_unlock(CARL *handle, carl_lock_data data, void *useptr)
{
  (void)handle;
  (void)data;
  (void)useptr;
  printf("<- Mutex unlock\n");
}

/* test function */
int test(char *URL)
{
  CARLcode res = CARLE_OK;
  CARLSH *share;
  int i;

  global_init(CARL_GLOBAL_ALL);

  share = carl_share_init();
  if(!share) {
    fprintf(stderr, "carl_share_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  carl_share_setopt(share, CARLSHOPT_SHARE, CARL_LOCK_DATA_CONNECT);
  carl_share_setopt(share, CARLSHOPT_LOCKFUNC, my_lock);
  carl_share_setopt(share, CARLSHOPT_UNLOCKFUNC, my_unlock);

  /* Loop the transfer and cleanup the handle properly every lap. This will
     still reuse connections since the pool is in the shared object! */

  for(i = 0; i < 3; i++) {
    CARL *carl = carl_easy_init();
    if(carl) {
      carl_easy_setopt(carl, CARLOPT_URL, URL);

      /* use the share object */
      carl_easy_setopt(carl, CARLOPT_SHARE, share);

      /* Perform the request, res will get the return code */
      res = carl_easy_perform(carl);
      /* Check for errors */
      if(res != CARLE_OK)
        fprintf(stderr, "carl_easy_perform() failed: %s\n",
                carl_easy_strerror(res));

      /* always cleanup */
      carl_easy_cleanup(carl);
    }
  }

  carl_share_cleanup(share);
  carl_global_cleanup();

  return 0;
}
