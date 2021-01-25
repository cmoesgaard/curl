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
  int res = 0;
  CARLcode easyret;
  CARLMcode multiret;
  CARLSHcode shareret;
  (void)URL;

  carl_easy_strerror((CARLcode)INT_MAX);
  carl_multi_strerror((CARLMcode)INT_MAX);
  carl_share_strerror((CARLSHcode)INT_MAX);
  carl_easy_strerror((CARLcode)-INT_MAX);
  carl_multi_strerror((CARLMcode)-INT_MAX);
  carl_share_strerror((CARLSHcode)-INT_MAX);
  for(easyret = CARLE_OK; easyret <= CARL_LAST; easyret++) {
    printf("e%d: %s\n", (int)easyret, carl_easy_strerror(easyret));
  }
  for(multiret = CARLM_CALL_MULTI_PERFORM; multiret <= CARLM_LAST;
      multiret++) {
    printf("m%d: %s\n", (int)multiret, carl_multi_strerror(multiret));
  }
  for(shareret = CARLSHE_OK; shareret <= CARLSHE_LAST; shareret++) {
    printf("s%d: %s\n", (int)shareret, carl_share_strerror(shareret));
  }

  return (int)res;
}
