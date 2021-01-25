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

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define TEST_HANG_TIMEOUT 60 * 1000

int test(char *URL)
{
  CARL *carls = NULL;
  CARLM *multi = NULL;
  int still_running;
  int i = TEST_ERR_FAILURE;
  int res = 0;
  CARLMsg *msg;

  start_test_timing();

  global_init(CARL_GLOBAL_ALL);

  multi_init(multi);

  easy_init(carls);

  easy_setopt(carls, CARLOPT_URL, URL);
  easy_setopt(carls, CARLOPT_HEADER, 1L);

  multi_add_handle(multi, carls);

  multi_perform(multi, &still_running);

  abort_on_test_timeout();

  while(still_running) {
    int num;
    res = carl_multi_wait(multi, NULL, 0, TEST_HANG_TIMEOUT, &num);
    if(res != CARLM_OK) {
      printf("carl_multi_wait() returned %d\n", res);
      res = TEST_ERR_MAJOR_BAD;
      goto test_cleanup;
    }

    abort_on_test_timeout();

    multi_perform(multi, &still_running);

    abort_on_test_timeout();
  }

  msg = carl_multi_info_read(multi, &still_running);
  if(msg)
    /* this should now contain a result code from the easy handle,
       get it */
    i = msg->data.result;

test_cleanup:

  /* undocumented cleanup sequence - type UA */

  carl_multi_cleanup(multi);
  carl_easy_cleanup(carls);
  carl_global_cleanup();

  if(res)
    i = res;

  return i; /* return the final return code */
}
