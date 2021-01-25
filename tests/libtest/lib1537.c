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

int test(char *URL)
{
  const unsigned char a[] = {0x2f, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
                             0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7};
  CARLcode res = CARLE_OK;
  char *ptr = NULL;
  int asize;
  int outlen = 0;
  char *raw;

  (void)URL; /* we don't use this */

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  asize = (int)sizeof(a);
  ptr = carl_easy_escape(NULL, (char *)a, asize);
  printf("%s\n", ptr);
  carl_free(ptr);

  /* deprecated API */
  ptr = carl_escape((char *)a, asize);
  printf("%s\n", ptr);
  if(!ptr) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }

  raw = carl_easy_unescape(NULL, ptr, (int)strlen(ptr), &outlen);
  printf("outlen == %d\n", outlen);
  printf("unescape == original? %s\n",
         memcmp(raw, a, outlen) ? "no" : "YES");
  carl_free(raw);

  /* deprecated API */
  raw = carl_unescape(ptr, (int)strlen(ptr));
  if(!raw) {
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  outlen = (int)strlen(raw);
  printf("[old] outlen == %d\n", outlen);
  printf("[old] unescape == original? %s\n",
         memcmp(raw, a, outlen) ? "no" : "YES");
  carl_free(raw);
  carl_free(ptr);

  /* weird input length */
  ptr = carl_easy_escape(NULL, (char *)a, -1);
  printf("escape -1 length: %s\n", ptr);

  /* weird input length */
  outlen = 2017; /* just a value */
  ptr = carl_easy_unescape(NULL, (char *)"moahahaha", -1, &outlen);
  printf("unescape -1 length: %s %d\n", ptr, outlen);

test_cleanup:
  carl_free(ptr);
  carl_global_cleanup();

  return (int)res;
}
