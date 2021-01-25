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

/*
 * Test a simple OPTIONS request with a custom header
 */
int test(char *URL)
{
  CARLcode res;
  CARL *carl;
  struct carl_slist *custom_headers = NULL;

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* Dump data to stdout for protocol verification */
  test_setopt(carl, CARLOPT_HEADERDATA, stdout);
  test_setopt(carl, CARLOPT_WRITEDATA, stdout);

  test_setopt(carl, CARLOPT_URL, URL);
  test_setopt(carl, CARLOPT_RTSP_STREAM_URI, URL);
  test_setopt(carl, CARLOPT_RTSP_REQUEST, CARL_RTSPREQ_OPTIONS);
  test_setopt(carl, CARLOPT_USERAGENT, "test567");

  custom_headers = carl_slist_append(custom_headers, "Test-Number: 567");
  test_setopt(carl, CARLOPT_RTSPHEADER, custom_headers);

  res = carl_easy_perform(carl);

test_cleanup:

  if(custom_headers)
    carl_slist_free_all(custom_headers);
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)res;
}
