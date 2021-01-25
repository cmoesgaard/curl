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
#include "carlcheck.h"

#include "llist.h"

static CARL *easy;

static CARLcode unit_setup(void)
{
  int res = CARLE_OK;

  global_init(CARL_GLOBAL_ALL);
  easy = carl_easy_init();
  if(!easy) {
    carl_global_cleanup();
    return CARLE_OUT_OF_MEMORY;
  }
  return res;
}

static void unit_stop(void)
{
  carl_easy_cleanup(easy);
  carl_global_cleanup();
}

UNITTEST_START
  int len;
  char *esc;

  esc = carl_easy_escape(easy, "", -1);
  fail_unless(esc == NULL, "negative string length can't work");

  esc = carl_easy_unescape(easy, "%41%41%41%41", -1, &len);
  fail_unless(esc == NULL, "negative string length can't work");

UNITTEST_STOP
