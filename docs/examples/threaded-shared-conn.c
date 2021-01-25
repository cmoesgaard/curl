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
/* <DESC>
 * Multi-threaded transfers sharing a single connection pool
 * </DESC>
 *
 * This example fires up NUM_THREADS threads and in each single thread, it
 * downloads the same fixed URL a URL_ITERATIONS number of times. The received
 * data is just thrown away. It sets up a single shared object that holds the
 * connection cache and all easy handles in all threads share that same cache.
 *
 * This example uses pthreads for threads and mutexes, but should be easy to
 * modify to use different thread/mutex system should you want to.
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <carl/carl.h>

/*
  URL to fetch. If you select HTTPS, you need to use a TLS backend with mutex
  locks taken care of (OpenSSL 1.1.x, NSS, etc) or add SSL mutex callbacks!
*/
#define URL "http://localhost/4KB"

/* number of threads to fire up in parallel */
#define NUM_THREADS 67

/* how many times each URL is transferred per thread */
#define URL_ITERATIONS 11235

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

  for(i = 0; i < URL_ITERATIONS; i++) {
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

int main(void)
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
