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
 * This unit test PUT http data over proxy. Proxy header will be different
 * from server http header
 */

#include "test.h"
#include <stdio.h>
#include "memdebug.h"

static char data [] = "Hello Cloud!\r\n";
static size_t consumed = 0;

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t  amount = nmemb * size; /* Total bytes carl wants */

  if(consumed == strlen(data)) {
    return 0;
  }

  if(amount > strlen(data)-consumed) {
    amount = strlen(data);
  }

  consumed += amount;
  (void)stream;
  memcpy(ptr, data, amount);
  return amount;
}

/*
 * carefully not leak memory on OOM
 */
static int trailers_callback(struct carl_slist **list, void *userdata)
{
  struct carl_slist *nlist = NULL;
  struct carl_slist *nlist2 = NULL;
  (void)userdata;
  nlist = carl_slist_append(*list, "my-super-awesome-trailer: trail1");
  if(nlist)
    nlist2 = carl_slist_append(nlist, "my-other-awesome-trailer: trail2");
  if(nlist2) {
    *list = nlist2;
    return CARL_TRAILERFUNC_OK;
  }
  else {
    carl_slist_free_all(nlist);
    return CARL_TRAILERFUNC_ABORT;
  }
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

  hhl = carl_slist_append(hhl, "Trailer: my-super-awesome-trailer,"
                               " my-other-awesome-trailer");
  if(!hhl) {
    goto test_cleanup;
  }

  test_setopt(carl, CARLOPT_URL, URL);
  test_setopt(carl, CARLOPT_HTTPHEADER, hhl);
  test_setopt(carl, CARLOPT_PUT, 1L);
  test_setopt(carl, CARLOPT_READFUNCTION, read_callback);
  test_setopt(carl, CARLOPT_TRAILERFUNCTION, trailers_callback);
  test_setopt(carl, CARLOPT_TRAILERDATA, NULL);

  res = carl_easy_perform(carl);

test_cleanup:

  carl_easy_cleanup(carl);

  carl_slist_free_all(hhl);

  carl_global_cleanup();

  return (int)res;
}
