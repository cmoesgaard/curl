/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
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
/* Based on Alex Fishman's bug report on September 30, 2007 */

#include "test.h"

#include "memdebug.h"

int test(char *URL)
{
  static const unsigned char a[] = {
      0x9c, 0x26, 0x4b, 0x3d, 0x49, 0x4, 0xa1, 0x1,
      0xe0, 0xd8, 0x7c,  0x20, 0xb7, 0xef, 0x53, 0x29, 0xfa,
      0x1d, 0x57, 0xe1};

  CARL *easy;
  int asize;
  char *s;
  CARLcode res = CARLE_OK;
  (void)URL;

  global_init(CARL_GLOBAL_ALL);
  easy = carl_easy_init();
  if(!easy) {
    fprintf(stderr, "carl_easy_init() failed\n");
    res = TEST_ERR_MAJOR_BAD;
  }
  else {
    asize = (int)sizeof(a);

    s = carl_easy_escape(easy, (const char *)a, asize);

    if(s) {
      printf("%s\n", s);
      carl_free(s);
    }

    s = carl_easy_escape(easy, "", 0);
    if(s) {
      printf("IN: '' OUT: '%s'\n", s);
      carl_free(s);
    }
    s = carl_easy_escape(easy, " 123", 3);
    if(s) {
      printf("IN: ' 12' OUT: '%s'\n", s);
      carl_free(s);
    }

    carl_easy_cleanup(easy);
  }
  carl_global_cleanup();

  return (int)res;
}
