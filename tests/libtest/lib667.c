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

static char data[]=
#ifdef CARL_DOES_CONVERSIONS
  /* ASCII representation with escape sequences for non-ASCII platforms */
  "\x64\x75\x6d\x6d\x79";
#else
  "dummy";
#endif

struct WriteThis {
  char *readptr;
  carl_off_t sizeleft;
};

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *pooh = (struct WriteThis *)userp;
  int eof = !*pooh->readptr;

  if(size*nmemb < 1)
    return 0;

  eof = pooh->sizeleft <= 0;
  if(!eof)
    pooh->sizeleft--;

  if(!eof) {
    *ptr = *pooh->readptr;           /* copy one single byte */
    pooh->readptr++;                 /* advance pointer */
    return 1;                        /* we return 1 byte at a time! */
  }

  return 0;                         /* no more data left to deliver */
}

int test(char *URL)
{
  CARL *easy = NULL;
  carl_mime *mime = NULL;
  carl_mimepart *part;
  CARLcode result;
  int res = TEST_ERR_FAILURE;
  struct WriteThis pooh;

  /*
   * Check proper handling of mime encoder feature when the part read callback
   * delivers data bytes one at a time. Use chunked encoding for accurate test.
   */

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  easy = carl_easy_init();

  /* First set the URL that is about to receive our POST. */
  test_setopt(easy, CARLOPT_URL, URL);

  /* get verbose debug output please */
  test_setopt(easy, CARLOPT_VERBOSE, 1L);

  /* include headers in the output */
  test_setopt(easy, CARLOPT_HEADER, 1L);

  /* Prepare the callback structure. */
  pooh.readptr = data;
  pooh.sizeleft = (carl_off_t) strlen(data);

  /* Build the mime tree. */
  mime = carl_mime_init(easy);
  part = carl_mime_addpart(mime);
  carl_mime_name(part, "field");
  carl_mime_encoder(part, "base64");
  /* Using an undefined length forces chunked transfer. */
  carl_mime_data_cb(part, (carl_off_t) -1, read_callback, NULL, NULL, &pooh);

  /* Bind mime data to its easy handle. */
  test_setopt(easy, CARLOPT_MIMEPOST, mime);

  /* Send data. */
  result = carl_easy_perform(easy);
  if(result) {
    fprintf(stderr, "carl_easy_perform() failed\n");
    res = (int) result;
  }

test_cleanup:
  carl_easy_cleanup(easy);
  carl_mime_free(mime);
  carl_global_cleanup();
  return res;
}
