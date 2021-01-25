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

static const char *HOSTHEADER = "Host: www.host.foo.com";
static const char *JAR = "log/jar506";
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

static int locks[3];

/* lock callback */
static void my_lock(CARL *handle, carl_lock_data data,
                    carl_lock_access laccess, void *useptr)
{
  const char *what;
  struct userdata *user = (struct userdata *)useptr;
  int locknum;

  (void)handle;
  (void)laccess;

  switch(data) {
    case CARL_LOCK_DATA_SHARE:
      what = "share";
      locknum = 0;
      break;
    case CARL_LOCK_DATA_DNS:
      what = "dns";
      locknum = 1;
      break;
    case CARL_LOCK_DATA_COOKIE:
      what = "cookie";
      locknum = 2;
      break;
    default:
      fprintf(stderr, "lock: no such data: %d\n", (int)data);
      return;
  }

  /* detect locking of locked locks */
  if(locks[locknum]) {
    printf("lock: double locked %s\n", what);
    return;
  }
  locks[locknum]++;

  printf("lock:   %-6s [%s]: %d\n", what, user->text, user->counter);
  user->counter++;
}

/* unlock callback */
static void my_unlock(CARL *handle, carl_lock_data data, void *useptr)
{
  const char *what;
  struct userdata *user = (struct userdata *)useptr;
  int locknum;
  (void)handle;
  switch(data) {
    case CARL_LOCK_DATA_SHARE:
      what = "share";
      locknum = 0;
      break;
    case CARL_LOCK_DATA_DNS:
      what = "dns";
      locknum = 1;
      break;
    case CARL_LOCK_DATA_COOKIE:
      what = "cookie";
      locknum = 2;
      break;
    default:
      fprintf(stderr, "unlock: no such data: %d\n", (int)data);
      return;
  }

  /* detect unlocking of unlocked locks */
  if(!locks[locknum]) {
    printf("unlock: double unlocked %s\n", what);
    return;
  }
  locks[locknum]--;

  printf("unlock: %-6s [%s]: %d\n", what, user->text, user->counter);
  user->counter++;
}


/* build host entry */
static struct carl_slist *sethost(struct carl_slist *headers)
{
  (void)headers;
  return carl_slist_append(NULL, HOSTHEADER);
}


/* the dummy thread function */
static void *fire(void *ptr)
{
  CARLcode code;
  struct carl_slist *headers;
  struct Tdata *tdata = (struct Tdata*)ptr;
  CARL *carl;

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    return NULL;
  }

  headers = sethost(NULL);
  carl_easy_setopt(carl, CARLOPT_VERBOSE,    1L);
  carl_easy_setopt(carl, CARLOPT_HTTPHEADER, headers);
  carl_easy_setopt(carl, CARLOPT_URL,        tdata->url);
  carl_easy_setopt(carl, CARLOPT_COOKIEFILE, "");
  printf("CARLOPT_SHARE\n");
  carl_easy_setopt(carl, CARLOPT_SHARE, tdata->share);

  printf("PERFORM\n");
  code = carl_easy_perform(carl);
  if(code) {
    int i = 0;
    fprintf(stderr, "perform url '%s' repeat %d failed, carlcode %d\n",
            tdata->url, i, (int)code);
  }

  printf("CLEANUP\n");
  carl_easy_cleanup(carl);
  carl_slist_free_all(headers);

  return NULL;
}


/* build request url */
static char *suburl(const char *base, int i)
{
  return carl_maprintf("%s%.4d", base, i);
}


/* test function */
int test(char *URL)
{
  int res;
  CARLSHcode scode = CARLSHE_OK;
  CARLcode code = CARLE_OK;
  char *url = NULL;
  struct Tdata tdata;
  CARL *carl;
  CARLSH *share;
  struct carl_slist *headers = NULL;
  struct carl_slist *cookies = NULL;
  struct carl_slist *next_cookie = NULL;
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
    printf("CARL_LOCK_DATA_COOKIE\n");
    scode = carl_share_setopt(share, CARLSHOPT_SHARE, CARL_LOCK_DATA_COOKIE);
  }
  if(CARLSHE_OK == scode) {
    printf("CARL_LOCK_DATA_DNS\n");
    scode = carl_share_setopt(share, CARLSHOPT_SHARE, CARL_LOCK_DATA_DNS);
  }

  if(CARLSHE_OK != scode) {
    fprintf(stderr, "carl_share_setopt() failed\n");
    carl_share_cleanup(share);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* initial cookie manipulation */
  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_share_cleanup(share);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }
  printf("CARLOPT_SHARE\n");
  test_setopt(carl, CARLOPT_SHARE,      share);
  printf("CARLOPT_COOKIELIST injected_and_clobbered\n");
  test_setopt(carl, CARLOPT_COOKIELIST,
               "Set-Cookie: injected_and_clobbered=yes; "
               "domain=host.foo.com; expires=Sat Feb 2 11:56:27 GMT 2030");
  printf("CARLOPT_COOKIELIST ALL\n");
  test_setopt(carl, CARLOPT_COOKIELIST, "ALL");
  printf("CARLOPT_COOKIELIST session\n");
  test_setopt(carl, CARLOPT_COOKIELIST, "Set-Cookie: session=elephants");
  printf("CARLOPT_COOKIELIST injected\n");
  test_setopt(carl, CARLOPT_COOKIELIST,
               "Set-Cookie: injected=yes; domain=host.foo.com; "
               "expires=Sat Feb 2 11:56:27 GMT 2030");
  printf("CARLOPT_COOKIELIST SESS\n");
  test_setopt(carl, CARLOPT_COOKIELIST, "SESS");
  printf("CLEANUP\n");
  carl_easy_cleanup(carl);


  res = 0;

  /* start treads */
  for(i = 1; i <= THREADS; i++) {

    /* set thread data */
    tdata.url   = suburl(URL, i); /* must be carl_free()d */
    tdata.share = share;

    /* simulate thread, direct call of "thread" function */
    printf("*** run %d\n",i);
    fire(&tdata);

    carl_free(tdata.url);
  }


  /* fetch a another one and save cookies */
  printf("*** run %d\n", i);
  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_share_cleanup(share);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  url = suburl(URL, i);
  headers = sethost(NULL);
  test_setopt(carl, CARLOPT_HTTPHEADER, headers);
  test_setopt(carl, CARLOPT_URL,        url);
  printf("CARLOPT_SHARE\n");
  test_setopt(carl, CARLOPT_SHARE,      share);
  printf("CARLOPT_COOKIEJAR\n");
  test_setopt(carl, CARLOPT_COOKIEJAR,  JAR);
  printf("CARLOPT_COOKIELIST FLUSH\n");
  test_setopt(carl, CARLOPT_COOKIELIST, "FLUSH");

  printf("PERFORM\n");
  carl_easy_perform(carl);

  printf("CLEANUP\n");
  carl_easy_cleanup(carl);
  carl_free(url);
  carl_slist_free_all(headers);

  /* load cookies */
  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_share_cleanup(share);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }
  url = suburl(URL, i);
  headers = sethost(NULL);
  test_setopt(carl, CARLOPT_HTTPHEADER, headers);
  test_setopt(carl, CARLOPT_URL,        url);
  printf("CARLOPT_SHARE\n");
  test_setopt(carl, CARLOPT_SHARE,      share);
  printf("CARLOPT_COOKIELIST ALL\n");
  test_setopt(carl, CARLOPT_COOKIELIST, "ALL");
  printf("CARLOPT_COOKIEJAR\n");
  test_setopt(carl, CARLOPT_COOKIEFILE, JAR);
  printf("CARLOPT_COOKIELIST RELOAD\n");
  test_setopt(carl, CARLOPT_COOKIELIST, "RELOAD");

  code = carl_easy_getinfo(carl, CARLINFO_COOKIELIST, &cookies);
  if(code != CARLE_OK) {
    fprintf(stderr, "carl_easy_getinfo() failed\n");
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  printf("loaded cookies:\n");
  if(!cookies) {
    fprintf(stderr, "  reloading cookies from '%s' failed\n", JAR);
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }
  printf("-----------------\n");
  next_cookie = cookies;
  while(next_cookie) {
    printf("  %s\n", next_cookie->data);
    next_cookie = next_cookie->next;
  }
  printf("-----------------\n");
  carl_slist_free_all(cookies);

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
  carl_slist_free_all(headers);
  carl_free(url);

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
