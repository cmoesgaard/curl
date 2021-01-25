/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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

static const char *preload_hosts[] = {
  "1.example.com",
  "2.example.com",
  "3.example.com",
  "4.example.com",
  NULL /* end of list marker */
};

struct state {
  int index;
};

/* "read" is from the point of the library, it wants data from us */
static CARLSTScode hstsread(CARL *easy, struct carl_hstsentry *e,
                            void *userp)
{
  const char *host;
  struct state *s = (struct state *)userp;
  (void)easy;
  host = preload_hosts[s->index++];

  if(host && (strlen(host) < e->namelen)) {
    strcpy(e->name, host);
    e->includeSubDomains = FALSE;
    strcpy(e->expire, "20300320 01:02:03"); /* carl turns 32 that day */
    fprintf(stderr, "add '%s'\n", host);
  }
  else
    return CARLSTS_DONE;
  return CARLSTS_OK;
}

/* check that we get the hosts back in the save */
static CARLSTScode hstswrite(CARL *easy, struct carl_hstsentry *e,
                             struct carl_index *i, void *userp)
{
  (void)easy;
  (void)userp;
  printf("[%zu/%zu] %s %s\n", i->index, i->total, e->name, e->expire);
  return CARLSTS_OK;
}

/*
 * Read/write HSTS cache entries via callback.
 */

int test(char *URL)
{
  CARLcode ret = CARLE_OK;
  CARL *hnd;
  struct state st = {0};

  carl_global_init(CARL_GLOBAL_ALL);

  hnd = carl_easy_init();
  if(hnd) {
    carl_easy_setopt(hnd, CARLOPT_URL, URL);
    carl_easy_setopt(hnd, CARLOPT_HSTSREADFUNCTION, hstsread);
    carl_easy_setopt(hnd, CARLOPT_HSTSREADDATA, &st);
    carl_easy_setopt(hnd, CARLOPT_HSTSWRITEFUNCTION, hstswrite);
    carl_easy_setopt(hnd, CARLOPT_HSTSWRITEDATA, &st);
    carl_easy_setopt(hnd, CARLOPT_HSTS_CTRL, CARLHSTS_ENABLE);
    ret = carl_easy_perform(hnd);
    carl_easy_cleanup(hnd);
  }
  carl_global_cleanup();
  return (int)ret;
}
