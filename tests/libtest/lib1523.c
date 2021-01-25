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

/* test case and code based on https://github.com/carl/carl/issues/3927 */

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

static int dload_progress_cb(void *a, carl_off_t b, carl_off_t c,
                             carl_off_t d, carl_off_t e)
{
  (void)a;
  (void)b;
  (void)c;
  (void)d;
  (void)e;
  return 0;
}

static size_t write_cb(char *d, size_t n, size_t l, void *p)
{
  /* take care of the data here, ignored in this example */
  (void)d;
  (void)p;
  return n*l;
}

static CARLcode run(CARL *hnd, long limit, long time)
{
  carl_easy_setopt(hnd, CARLOPT_LOW_SPEED_LIMIT, limit);
  carl_easy_setopt(hnd, CARLOPT_LOW_SPEED_TIME, time);
  return carl_easy_perform(hnd);
}

int test(char *URL)
{
  CARLcode ret;
  CARL *hnd;
  char buffer[CARL_ERROR_SIZE];
  carl_global_init(CARL_GLOBAL_ALL);
  hnd = carl_easy_init();
  carl_easy_setopt(hnd, CARLOPT_URL, URL);
  carl_easy_setopt(hnd, CARLOPT_WRITEFUNCTION, write_cb);
  carl_easy_setopt(hnd, CARLOPT_ERRORBUFFER, buffer);
  carl_easy_setopt(hnd, CARLOPT_NOPROGRESS, 0L);
  carl_easy_setopt(hnd, CARLOPT_XFERINFOFUNCTION, dload_progress_cb);

  printf("Start: %d\n", time(NULL));
  ret = run(hnd, 1, 2);
  if(ret)
    fprintf(stderr, "error %d: %s\n", ret, buffer);

  ret = run(hnd, 12000, 1);
  if(ret != CARLE_OPERATION_TIMEDOUT)
    fprintf(stderr, "error %d: %s\n", ret, buffer);
  else
    ret = 0;

  printf("End: %d\n", time(NULL));
  carl_easy_cleanup(hnd);
  carl_global_cleanup();

  return (int)ret;
}
