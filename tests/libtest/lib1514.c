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
 * Make sure libcarl does not send a `Content-Length: -1` header when HTTP POST
 * size is unknown.
 */

#include "test.h"

#include "memdebug.h"

static char data[]="dummy";

struct WriteThis {
  char *readptr;
  size_t sizeleft;
};

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *pooh = (struct WriteThis *)userp;

  if(size*nmemb < 1)
    return 0;

  if(pooh->sizeleft) {
    *ptr = pooh->readptr[0]; /* copy one single byte */
    pooh->readptr++;                 /* advance pointer */
    pooh->sizeleft--;                /* less data left */
    return 1;                        /* we return 1 byte at a time! */
  }

  return 0;                         /* no more data left to deliver */
}

int test(char *URL)
{
  CARL *carl;
  CARLcode result = CARLE_OK;
  int res = 0;
  struct WriteThis pooh = { data, sizeof(data)-1 };

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_URL, URL);
  easy_setopt(carl, CARLOPT_POST, 1L);
  /* Purposely omit to set CARLOPT_POSTFIELDSIZE */
  easy_setopt(carl, CARLOPT_READFUNCTION, read_callback);
  easy_setopt(carl, CARLOPT_READDATA, &pooh);
#ifdef LIB1539
  /* speak HTTP 1.0 - no chunked! */
  easy_setopt(carl, CARLOPT_HTTP_VERSION, CARL_HTTP_VERSION_1_0);
#endif

  result = carl_easy_perform(carl);

test_cleanup:

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)result;
}
