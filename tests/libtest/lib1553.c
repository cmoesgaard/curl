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

static int xferinfo(void *p,
                    carl_off_t dltotal, carl_off_t dlnow,
                    carl_off_t ultotal, carl_off_t ulnow)
{
  (void)p;
  (void)dlnow;
  (void)dltotal;
  (void)ulnow;
  (void)ultotal;
  fprintf(stderr, "xferinfo fail!\n");
  return 1; /* fail as fast as we can */
}

int test(char *URL)
{
  CARL *carls = NULL;
  CARLM *multi = NULL;
  int still_running;
  int i = 0;
  int res = 0;
  carl_mimepart *field = NULL;
  carl_mime *mime = NULL;
  int counter = 1;

  start_test_timing();

  global_init(CARL_GLOBAL_ALL);

  multi_init(multi);

  easy_init(carls);

  mime = carl_mime_init(carls);
  field = carl_mime_addpart(mime);
  carl_mime_name(field, "name");
  carl_mime_data(field, "value", CARL_ZERO_TERMINATED);

  easy_setopt(carls, CARLOPT_URL, URL);
  easy_setopt(carls, CARLOPT_HEADER, 1L);
  easy_setopt(carls, CARLOPT_VERBOSE, 1L);
  easy_setopt(carls, CARLOPT_MIMEPOST, mime);
  easy_setopt(carls, CARLOPT_USERPWD, "u:s");
  easy_setopt(carls, CARLOPT_XFERINFOFUNCTION, xferinfo);
  easy_setopt(carls, CARLOPT_NOPROGRESS, 1L);

  multi_add_handle(multi, carls);

  multi_perform(multi, &still_running);

  abort_on_test_timeout();

  while(still_running && counter--) {
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

test_cleanup:

  carl_mime_free(mime);
  carl_multi_remove_handle(multi, carls);
  carl_multi_cleanup(multi);
  carl_easy_cleanup(carls);
  carl_global_cleanup();

  if(res)
    i = res;

  return i; /* return the final return code */
}
