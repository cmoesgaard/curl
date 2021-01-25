/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2019 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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
/*
 * KNOW_BUGS "A shared connection cache is not thread-safe"
 *
 * This source code was used to verify shared connection cache but since this
 * is a known issue the test is no longer built or run. This code is here to
 * allow for testing once someone gets to work on fixing this.
 */
#include "test.h"

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#include <time.h>

/* number of threads to fire up in parallel */
#define NUM_THREADS 67

/* for how many seconds each thread will loop */
#define RUN_FOR_SECONDS 7

static pthread_mutex_t connlock;

static size_t write_db(void *ptr, size_t size, size_t nmemb, void *data)
{
  /* not interested in the downloaded bytes, return the size */
  (void)ptr;  /* unused */
  (void)data; /* unused */
  return (size_t)(size * nmemb);
}

static void lock_cb(CARL *handle, carl_lock_data data,
                    carl_lock_access access, void *userptr)
{
  (void)access; /* unused */
  (void)userptr; /* unused */
  (void)handle; /* unused */
  (void)data; /* unused */
  pthread_mutex_lock(&connlock);
}

static void unlock_cb(CARL *handle, carl_lock_data data,
                      void *userptr)
{
  (void)userptr; /* unused */
  (void)handle;  /* unused */
  (void)data;    /* unused */
  pthread_mutex_unlock(&connlock);
}

static void init_locks(void)
{
  pthread_mutex_init(&connlock, NULL);
}

static void kill_locks(void)
{
  pthread_mutex_destroy(&connlock);
}

struct initurl {
  const char *url;
  CARLSH *share;
  int threadno;
};

static void *run_thread(void *ptr)
{
  struct initurl *u = (struct initurl *)ptr;
  int i;
  time_t end = time(NULL) + RUN_FOR_SECONDS;

  for(i = 0; time(NULL) < end; i++) {
    CARL *carl = carl_easy_init();
    carl_easy_setopt(carl, CARLOPT_URL, u->url);
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 0L);
    carl_easy_setopt(carl, CARLOPT_SHARE, u->share);
    carl_easy_setopt(carl, CARLOPT_WRITEFUNCTION, write_db);
    carl_easy_perform(carl); /* ignores error */
    carl_easy_cleanup(carl);
    fprintf(stderr, "Thread %d transfer %d\n", u->threadno, i);
  }

  return NULL;
}

int test(char *URL)
{
  pthread_t tid[NUM_THREADS];
  int i;
  CARLSH *share;
  struct initurl url[NUM_THREADS];

  /* Must initialize libcarl before any threads are started */
  carl_global_init(CARL_GLOBAL_ALL);

  share = carl_share_init();
  carl_share_setopt(share, CARLSHOPT_LOCKFUNC, lock_cb);
  carl_share_setopt(share, CARLSHOPT_UNLOCKFUNC, unlock_cb);
  carl_share_setopt(share, CARLSHOPT_SHARE, CARL_LOCK_DATA_CONNECT);

  init_locks();

  for(i = 0; i< NUM_THREADS; i++) {
    int error;
    url[i].url = URL;
    url[i].share = share;
    url[i].threadno = i;
    error = pthread_create(&tid[i], NULL, run_thread, &url[i]);
    if(0 != error)
      fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, error);
    else
      fprintf(stderr, "Thread %d, gets %s\n", i, URL);
  }

  /* now wait for all threads to terminate */
  for(i = 0; i< NUM_THREADS; i++) {
    pthread_join(tid[i], NULL);
    fprintf(stderr, "Thread %d terminated\n", i);
  }

  kill_locks();

  carl_share_cleanup(share);
  carl_global_cleanup();
  return 0;
}

#else /* without pthread, this test doesn't work */
int test(char *URL)
{
  (void)URL;
  return 0;
}
#endif
