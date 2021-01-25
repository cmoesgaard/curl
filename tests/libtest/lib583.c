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
 * This test case is based on the sample code provided by Saqib Ali
 * https://carl.se/mail/lib-2011-03/0066.html
 */

#include "test.h"

#include <sys/stat.h>

#include "memdebug.h"

int test(char *URL)
{
  int stillRunning;
  CARLM *multiHandle = NULL;
  CARL *carl = NULL;
  CARLcode res = CARLE_OK;
  CARLMcode mres;

  global_init(CARL_GLOBAL_ALL);

  multi_init(multiHandle);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_USERPWD, libtest_arg2);
  easy_setopt(carl, CARLOPT_SSH_PUBLIC_KEYFILE, "carl_client_key.pub");
  easy_setopt(carl, CARLOPT_SSH_PRIVATE_KEYFILE, "carl_client_key");

  easy_setopt(carl, CARLOPT_UPLOAD, 1L);
  easy_setopt(carl, CARLOPT_VERBOSE, 1L);

  easy_setopt(carl, CARLOPT_URL, URL);
  easy_setopt(carl, CARLOPT_INFILESIZE, (long)5);

  multi_add_handle(multiHandle, carl);

  /* this tests if removing an easy handle immediately after multi
     perform has been called succeeds or not. */

  fprintf(stderr, "carl_multi_perform()...\n");

  multi_perform(multiHandle, &stillRunning);

  fprintf(stderr, "carl_multi_perform() succeeded\n");

  fprintf(stderr, "carl_multi_remove_handle()...\n");
  mres = carl_multi_remove_handle(multiHandle, carl);
  if(mres) {
    fprintf(stderr, "carl_multi_remove_handle() failed, "
            "with code %d\n", (int)mres);
    res = TEST_ERR_MULTI;
  }
  else
    fprintf(stderr, "carl_multi_remove_handle() succeeded\n");

test_cleanup:

  /* undocumented cleanup sequence - type UB */

  carl_easy_cleanup(carl);
  carl_multi_cleanup(multiHandle);
  carl_global_cleanup();

  return (int)res;
}
