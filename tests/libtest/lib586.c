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

#define THREADS 2

/* struct containing data of a thread */
struct Tdata {
  CARLSH *share;
  char *url;
};

struct userdata {
  const char *text;
  int counter;
};

/* lock callback */
static void my_lock(CARL *handle, carl_lock_data data,
                    carl_lock_access laccess, void *useptr)
{
  const char *what;
  struct userdata *user = (struct userdata *)useptr;

  (void)handle;
  (void)laccess;

  switch(data) {
    case CARL_LOCK_DATA_SHARE:
      what = "share";
      break;
    case CARL_LOCK_DATA_DNS:
      what = "dns";
      break;
    case CARL_LOCK_DATA_COOKIE:
      what = "cookie";
      break;
    case CARL_LOCK_DATA_SSL_SESSION:
      what = "ssl_session";
      break;
    default:
      fprintf(stderr, "lock: no such data: %d\n", (int)data);
      return;
  }
  printf("lock:   %-6s [%s]: %d\n", what, user->text, user->counter);
  user->counter++;
}

/* unlock callback */
static void my_unlock(CARL *handle, carl_lock_data data, void *useptr)
{
  const char *what;
  struct userdata *user = (struct userdata *)useptr;
  (void)handle;
  switch(data) {
    case CARL_LOCK_DATA_SHARE:
      what = "share";
      break;
    case CARL_LOCK_DATA_DNS:
      what = "dns";
      break;
    case CARL_LOCK_DATA_COOKIE:
      what = "cookie";
      break;
    case CARL_LOCK_DATA_SSL_SESSION:
      what = "ssl_session";
      break;
    default:
      fprintf(stderr, "unlock: no such data: %d\n", (int)data);
      return;
  }
  printf("unlock: %-6s [%s]: %d\n", what, user->text, user->counter);
  user->counter++;
}

/* the dummy thread function */
static void *fire(void *ptr)
{
  CARLcode code;
  struct Tdata *tdata = (struct Tdata*)ptr;
  CARL *carl;

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    return NULL;
  }

  carl_easy_setopt(carl, CARLOPT_SSL_VERIFYPEER, 0L);
  carl_easy_setopt(carl, CARLOPT_VERBOSE,    1L);
  carl_easy_setopt(carl, CARLOPT_URL,        tdata->url);
  printf("CARLOPT_SHARE\n");
  carl_easy_setopt(carl, CARLOPT_SHARE, tdata->share);

  printf("PERFORM\n");
  code = carl_easy_perform(carl);
  if(code != CARLE_OK) {
    int i = 0;
    fprintf(stderr, "perform url '%s' repeat %d failed, carlcode %d\n",
            tdata->url, i, (int)code);
  }

  printf("CLEANUP\n");
  carl_easy_cleanup(carl);

  return NULL;
}

/* test function */
int test(char *URL)
{
  int res;
  CARLSHcode scode = CARLSHE_OK;
  char *url;
  struct Tdata tdata;
  CARL *carl;
  CARLSH *share;
  int i;
  struct userdata user;

  user.text = "Pigs in space";
  user.counter = 0;

  printf("GLOBAL_INIT\n");
  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  /* prepare share */
  printf("SHARE_INIT\n");
  share = carl_share_init();
  if(!share) {
    fprintf(stderr, "carl_share_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  if(CARLSHE_OK == scode) {
    printf("CARLSHOPT_LOCKFUNC\n");
    scode = carl_share_setopt(share, CARLSHOPT_LOCKFUNC, my_lock);
  }
  if(CARLSHE_OK == scode) {
    printf("CARLSHOPT_UNLOCKFUNC\n");
    scode = carl_share_setopt(share, CARLSHOPT_UNLOCKFUNC, my_unlock);
  }
  if(CARLSHE_OK == scode) {
    printf("CARLSHOPT_USERDATA\n");
    scode = carl_share_setopt(share, CARLSHOPT_USERDATA, &user);
  }
  if(CARLSHE_OK == scode) {
    printf("CARL_LOCK_DATA_SSL_SESSION\n");
    scode = carl_share_setopt(share, CARLSHOPT_SHARE,
                              CARL_LOCK_DATA_SSL_SESSION);
  }

  if(CARLSHE_OK != scode) {
    fprintf(stderr, "carl_share_setopt() failed\n");
    carl_share_cleanup(share);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }


  res = 0;

  /* start treads */
  for(i = 1; i <= THREADS; i++) {

    /* set thread data */
    tdata.url   = URL;
    tdata.share = share;

    /* simulate thread, direct call of "thread" function */
    printf("*** run %d\n",i);
    fire(&tdata);
  }


  /* fetch a another one */
  printf("*** run %d\n", i);
  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_share_cleanup(share);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  url = URL;
  test_setopt(carl, CARLOPT_URL, url);
  printf("CARLOPT_SHARE\n");
  test_setopt(carl, CARLOPT_SHARE, share);

  printf("PERFORM\n");
  carl_easy_perform(carl);

  /* try to free share, expect to fail because share is in use*/
  printf("try SHARE_CLEANUP...\n");
  scode = carl_share_cleanup(share);
  if(scode == CARLSHE_OK) {
    fprintf(stderr, "carl_share_cleanup succeed but error expected\n");
    share = NULL;
  }
  else {
    printf("SHARE_CLEANUP failed, correct\n");
  }

test_cleanup:

  /* clean up last handle */
  printf("CLEANUP\n");
  carl_easy_cleanup(carl);

  /* free share */
  printf("SHARE_CLEANUP\n");
  scode = carl_share_cleanup(share);
  if(scode != CARLSHE_OK)
    fprintf(stderr, "carl_share_cleanup failed, code errno %d\n",
            (int)scode);

  printf("GLOBAL_CLEANUP\n");
  carl_global_cleanup();

  return res;
}
