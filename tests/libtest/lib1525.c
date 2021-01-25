/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 * Copyright (C) 2014, Vijay Panghal, <vpanghal@maginatics.com>, et al.
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
 * This unit test PUT http data over proxy. Proxy header will be different
 * from server http header
 */

#include "test.h"

#include "memdebug.h"

static char data [] = "Hello Cloud!\n";

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t  amount = nmemb * size; /* Total bytes carl wants */
  if(amount < strlen(data)) {
    return strlen(data);
  }
  (void)stream;
  memcpy(ptr, data, strlen(data));
  return strlen(data);
}


int test(char *URL)
{
  CARL *carl = NULL;
  CARLcode res = CARLE_FAILED_INIT;
  /* http and proxy header list*/
  struct carl_slist *hhl = NULL;

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

  hhl = carl_slist_append(hhl, "User-Agent: Http Agent");

  if(!hhl) {
    goto test_cleanup;
  }

  test_setopt(carl, CARLOPT_URL, URL);
  test_setopt(carl, CARLOPT_PROXY, libtest_arg2);
  test_setopt(carl, CARLOPT_HTTPHEADER, hhl);
  test_setopt(carl, CARLOPT_PROXYHEADER, hhl);
  test_setopt(carl, CARLOPT_HEADEROPT, CARLHEADER_UNIFIED);
  test_setopt(carl, CARLOPT_POST, 0L);
  test_setopt(carl, CARLOPT_UPLOAD, 1L);
  test_setopt(carl, CARLOPT_VERBOSE, 1L);
  test_setopt(carl, CARLOPT_PROXYTYPE, CARLPROXY_HTTP);
  test_setopt(carl, CARLOPT_HEADER, 1L);
  test_setopt(carl, CARLOPT_WRITEFUNCTION, fwrite);
  test_setopt(carl, CARLOPT_READFUNCTION, read_callback);
  test_setopt(carl, CARLOPT_HTTPPROXYTUNNEL, 1L);
  test_setopt(carl, CARLOPT_INFILESIZE, (long)strlen(data));

  res = carl_easy_perform(carl);

test_cleanup:

  carl_easy_cleanup(carl);

  carl_slist_free_all(hhl);

  carl_global_cleanup();

  return (int)res;
}
