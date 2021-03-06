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
#include "test.h"

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

int test(char *URL)
{
  const struct carl_easyoption *o;
  int error = 0;
  (void)URL;

  carl_global_init(CARL_GLOBAL_ALL);

  for(o = carl_easy_option_next(NULL);
      o;
      o = carl_easy_option_next(o)) {
    const struct carl_easyoption *ename =
      carl_easy_option_by_name(o->name);
    const struct carl_easyoption *eid =
      carl_easy_option_by_id(o->id);

    if(ename->id != o->id) {
      printf("name lookup id %d doesn't match %d\n",
             ename->id, o->id);
    }
    else if(eid->id != o->id) {
      printf("ID lookup %d doesn't match %d\n",
             ename->id, o->id);
    }
  }
  carl_global_cleanup();
  return error;
}
