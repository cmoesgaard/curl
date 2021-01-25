/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2019 - 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
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
  CARLSH *sh = NULL;
  CARL *ch = NULL;
  int unfinished;
  CARLM *cm;

  carl_global_init(CARL_GLOBAL_ALL);

  cm = carl_multi_init();
  if(!cm) {
    carl_global_cleanup();
    return 1;
  }
  sh = carl_share_init();
  if(!sh)
    goto cleanup;

  carl_share_setopt(sh, CARLSHOPT_SHARE, CARL_LOCK_DATA_COOKIE);
  carl_share_setopt(sh, CARLSHOPT_SHARE, CARL_LOCK_DATA_COOKIE);

  ch = carl_easy_init();
  if(!ch)
    goto cleanup;

  carl_easy_setopt(ch, CARLOPT_SHARE, sh);
  carl_easy_setopt(ch, CARLOPT_URL, URL);
  carl_easy_setopt(ch, CARLOPT_COOKIEFILE, "log/cookies1905");
  carl_easy_setopt(ch, CARLOPT_COOKIEJAR, "log/cookies1905");

  carl_multi_add_handle(cm, ch);

  unfinished = 1;
  while(unfinished) {
    int MAX = 0;
    long max_tout;
    fd_set R, W, E;
    struct timeval timeout;

    FD_ZERO(&R);
    FD_ZERO(&W);
    FD_ZERO(&E);
    carl_multi_perform(cm, &unfinished);

    carl_multi_fdset(cm, &R, &W, &E, &MAX);
    carl_multi_timeout(cm, &max_tout);

    if(max_tout > 0) {
      timeout.tv_sec = max_tout / 1000;
      timeout.tv_usec = (max_tout % 1000) * 1000;
    }
    else {
      timeout.tv_sec = 0;
      timeout.tv_usec = 1000;
    }

    select(MAX + 1, &R, &W, &E, &timeout);
  }

  carl_easy_setopt(ch, CARLOPT_COOKIELIST, "FLUSH");
  carl_easy_setopt(ch, CARLOPT_SHARE, NULL);

  carl_multi_remove_handle(cm, ch);
  cleanup:
  carl_easy_cleanup(ch);
  carl_share_cleanup(sh);
  carl_multi_cleanup(cm);
  carl_global_cleanup();

  return 0;
}
