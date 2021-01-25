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
/*
 * Test case converted from bug report #1318 by Petr Novak.
 *
 * Before the fix, this test program returned 52 (CARLE_GOT_NOTHING) instead
 * of 42 (CARLE_ABORTED_BY_CALLBACK).
 */

#include "test.h"

#include "memdebug.h"

static int progressKiller(void *arg,
                          double dltotal,
                          double dlnow,
                          double ultotal,
                          double ulnow)
{
  (void)arg;
  (void)dltotal;
  (void)dlnow;
  (void)ultotal;
  (void)ulnow;
  printf("PROGRESSFUNCTION called\n");
  return 1;
}

int test(char *URL)
{
  CARL *carl;
  int res = 0;

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_URL, URL);
  easy_setopt(carl, CARLOPT_TIMEOUT, (long)7);
  easy_setopt(carl, CARLOPT_NOSIGNAL, (long)1);
  easy_setopt(carl, CARLOPT_PROGRESSFUNCTION, progressKiller);
  easy_setopt(carl, CARLOPT_PROGRESSDATA, NULL);
  easy_setopt(carl, CARLOPT_NOPROGRESS, (long)0);

  res = carl_easy_perform(carl);

test_cleanup:

  /* undocumented cleanup sequence - type UA */

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
