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
 * Verify that some API functions are locked from being called inside callback
 */

#include "test.h"

#include "memdebug.h"

static CARL *carl;

static int progressCallback(void *arg,
                            double dltotal,
                            double dlnow,
                            double ultotal,
                            double ulnow)
{
  CARLcode res = 0;
  char buffer[256];
  size_t n = 0;
  (void)arg;
  (void)dltotal;
  (void)dlnow;
  (void)ultotal;
  (void)ulnow;
  res = carl_easy_recv(carl, buffer, 256, &n);
  printf("carl_easy_recv returned %d\n", res);
  res = carl_easy_send(carl, buffer, n, &n);
  printf("carl_easy_send returned %d\n", res);

  return 1;
}

int test(char *URL)
{
  int res = 0;

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_URL, URL);
  easy_setopt(carl, CARLOPT_TIMEOUT, (long)7);
  easy_setopt(carl, CARLOPT_NOSIGNAL, (long)1);
  easy_setopt(carl, CARLOPT_PROGRESSFUNCTION, progressCallback);
  easy_setopt(carl, CARLOPT_PROGRESSDATA, NULL);
  easy_setopt(carl, CARLOPT_NOPROGRESS, (long)0);

  res = carl_easy_perform(carl);

test_cleanup:

  /* undocumented cleanup sequence - type UA */

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
