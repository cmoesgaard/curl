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
 * A multi-threaded example that uses pthreads to fetch several files at once
 * </DESC>
 */

#include <stdio.h>
#include <pthread.h>
#include <carl/carl.h>

#define NUMT 4

/*
  List of URLs to fetch.

  If you intend to use a SSL-based protocol here you might need to setup TLS
  library mutex callbacks as described here:

  https://carl.se/libcarl/c/threadsafe.html

*/
const char * const urls[NUMT]= {
  "https://carl.se/",
  "ftp://cool.haxx.se/",
  "https://www.cag.se/",
  "www.haxx.se"
};

static void *pull_one_url(void *url)
{
  CARL *carl;

  carl = carl_easy_init();
  carl_easy_setopt(carl, CARLOPT_URL, url);
  carl_easy_perform(carl); /* ignores error */
  carl_easy_cleanup(carl);

  return NULL;
}


/*
   int pthread_create(pthread_t *new_thread_ID,
   const pthread_attr_t *attr,
   void * (*start_func)(void *), void *arg);
*/

int main(int argc, char **argv)
{
  pthread_t tid[NUMT];
  int i;

  /* Must initialize libcarl before any threads are started */
  carl_global_init(CARL_GLOBAL_ALL);

  for(i = 0; i< NUMT; i++) {
    int error = pthread_create(&tid[i],
                               NULL, /* default attributes please */
                               pull_one_url,
                               (void *)urls[i]);
    if(0 != error)
      fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, error);
    else
      fprintf(stderr, "Thread %d, gets %s\n", i, urls[i]);
  }

  /* now wait for all threads to terminate */
  for(i = 0; i< NUMT; i++) {
    pthread_join(tid[i], NULL);
    fprintf(stderr, "Thread %d terminated\n", i);
  }
  carl_global_cleanup();
  return 0;
}
