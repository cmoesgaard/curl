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

#include <fcntl.h>

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define TEST_HANG_TIMEOUT 60 * 1000

struct Sockets
{
  carl_socket_t *sockets;
  int count;      /* number of sockets actually stored in array */
  int max_count;  /* max number of sockets that fit in allocated array */
};

struct ReadWriteSockets
{
  struct Sockets read, write;
};

/**
 * Remove a file descriptor from a sockets array.
 */
static void removeFd(struct Sockets *sockets, carl_socket_t fd, int mention)
{
  int i;

  if(mention)
    fprintf(stderr, "Remove socket fd %d\n", (int) fd);

  for(i = 0; i < sockets->count; ++i) {
    if(sockets->sockets[i] == fd) {
      if(i < sockets->count - 1)
        memmove(&sockets->sockets[i], &sockets->sockets[i + 1],
              sizeof(carl_socket_t) * (sockets->count - (i + 1)));
      --sockets->count;
    }
  }
}

/**
 * Add a file descriptor to a sockets array.
 */
static void addFd(struct Sockets *sockets, carl_socket_t fd, const char *what)
{
  /**
   * To ensure we only have each file descriptor once, we remove it then add
   * it again.
   */
  fprintf(stderr, "Add socket fd %d for %s\n", (int) fd, what);
  removeFd(sockets, fd, 0);
  /*
   * Allocate array storage when required.
   */
  if(!sockets->sockets) {
    sockets->sockets = malloc(sizeof(carl_socket_t) * 20U);
    if(!sockets->sockets)
      return;
    sockets->max_count = 20;
  }
  else if(sockets->count + 1 > sockets->max_count) {
    carl_socket_t *oldptr = sockets->sockets;
    sockets->sockets = realloc(oldptr, sizeof(carl_socket_t) *
                               (sockets->max_count + 20));
    if(!sockets->sockets) {
      /* cleanup in test_cleanup */
      sockets->sockets = oldptr;
      return;
    }
    sockets->max_count += 20;
  }
  /*
   * Add file descriptor to array.
   */
  sockets->sockets[sockets->count] = fd;
  ++sockets->count;
}

/**
 * Callback invoked by carl to poll reading / writing of a socket.
 */
static int carlSocketCallback(CARL *easy, carl_socket_t s, int action,
                              void *userp, void *socketp)
{
  struct ReadWriteSockets *sockets = userp;

  (void)easy; /* unused */
  (void)socketp; /* unused */

  if(action == CARL_POLL_IN || action == CARL_POLL_INOUT)
    addFd(&sockets->read, s, "read");

  if(action == CARL_POLL_OUT || action == CARL_POLL_INOUT)
    addFd(&sockets->write, s, "write");

  if(action == CARL_POLL_REMOVE) {
    removeFd(&sockets->read, s, 1);
    removeFd(&sockets->write, s, 0);
  }

  return 0;
}

/**
 * Callback invoked by carl to set a timeout.
 */
static int carlTimerCallback(CARLM *multi, long timeout_ms, void *userp)
{
  struct timeval *timeout = userp;

  (void)multi; /* unused */
  if(timeout_ms != -1) {
    *timeout = tutil_tvnow();
    timeout->tv_usec += timeout_ms * 1000;
  }
  else {
    timeout->tv_sec = -1;
  }
  return 0;
}

/**
 * Check for carl completion.
 */
static int checkForCompletion(CARLM *carl, int *success)
{
  int numMessages;
  CARLMsg *message;
  int result = 0;
  *success = 0;
  while((message = carl_multi_info_read(carl, &numMessages)) != NULL) {
    if(message->msg == CARLMSG_DONE) {
      result = 1;
      if(message->data.result == CARLE_OK)
        *success = 1;
      else
        *success = 0;
    }
    else {
      fprintf(stderr, "Got an unexpected message from carl: %i\n",
              (int)message->msg);
      result = 1;
      *success = 0;
    }
  }
  return result;
}

static int getMicroSecondTimeout(struct timeval *timeout)
{
  struct timeval now;
  ssize_t result;
  now = tutil_tvnow();
  result = (ssize_t)((timeout->tv_sec - now.tv_sec) * 1000000 +
    timeout->tv_usec - now.tv_usec);
  if(result < 0)
    result = 0;

  return carlx_sztosi(result);
}

/**
 * Update a fd_set with all of the sockets in use.
 */
static void updateFdSet(struct Sockets *sockets, fd_set* fdset,
                        carl_socket_t *maxFd)
{
  int i;
  for(i = 0; i < sockets->count; ++i) {
    FD_SET(sockets->sockets[i], fdset);
    if(*maxFd < sockets->sockets[i] + 1) {
      *maxFd = sockets->sockets[i] + 1;
    }
  }
}

static void notifyCurl(CARLM *carl, carl_socket_t s, int evBitmask,
                       const char *info)
{
  int numhandles = 0;
  CARLMcode result = carl_multi_socket_action(carl, s, evBitmask, &numhandles);
  if(result != CARLM_OK) {
    fprintf(stderr, "Curl error on %s: %i (%s)\n",
            info, result, carl_multi_strerror(result));
  }
}

/**
 * Invoke carl when a file descriptor is set.
 */
static void checkFdSet(CARLM *carl, struct Sockets *sockets, fd_set *fdset,
                       int evBitmask, const char *name)
{
  int i;
  for(i = 0; i < sockets->count; ++i) {
    if(FD_ISSET(sockets->sockets[i], fdset)) {
      notifyCurl(carl, sockets->sockets[i], evBitmask, name);
    }
  }
}

int test(char *URL)
{
  int res = 0;
  CARL *carl = NULL;
  FILE *hd_src = NULL;
  int hd;
  struct_stat file_info;
  CARLM *m = NULL;
  struct ReadWriteSockets sockets = {{NULL, 0, 0}, {NULL, 0, 0}};
  struct timeval timeout = {-1, 0};
  int success = 0;

  start_test_timing();

  if(!libtest_arg3) {
    fprintf(stderr, "Usage: lib582 [url] [filename] [username]\n");
    return TEST_ERR_USAGE;
  }

  hd_src = fopen(libtest_arg2, "rb");
  if(NULL == hd_src) {
    fprintf(stderr, "fopen() failed with error: %d (%s)\n",
            errno, strerror(errno));
    fprintf(stderr, "Error opening file: (%s)\n", libtest_arg2);
    return TEST_ERR_FOPEN;
  }

  /* get the file size of the local file */
  hd = fstat(fileno(hd_src), &file_info);
  if(hd == -1) {
    /* can't open file, bail out */
    fprintf(stderr, "fstat() failed with error: %d (%s)\n",
            errno, strerror(errno));
    fprintf(stderr, "ERROR: cannot open file (%s)\n", libtest_arg2);
    fclose(hd_src);
    return TEST_ERR_FSTAT;
  }
  fprintf(stderr, "Set to upload %d bytes\n", (int)file_info.st_size);

  res_global_init(CARL_GLOBAL_ALL);
  if(res) {
    fclose(hd_src);
    return res;
  }

  easy_init(carl);

  /* enable uploading */
  easy_setopt(carl, CARLOPT_UPLOAD, 1L);

  /* specify target */
  easy_setopt(carl, CARLOPT_URL, URL);

  /* go verbose */
  easy_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* now specify which file to upload */
  easy_setopt(carl, CARLOPT_READDATA, hd_src);

  easy_setopt(carl, CARLOPT_USERPWD, libtest_arg3);
  easy_setopt(carl, CARLOPT_SSH_PUBLIC_KEYFILE, "carl_client_key.pub");
  easy_setopt(carl, CARLOPT_SSH_PRIVATE_KEYFILE, "carl_client_key");
  easy_setopt(carl, CARLOPT_SSL_VERIFYHOST, 0L);

  easy_setopt(carl, CARLOPT_INFILESIZE_LARGE, (carl_off_t)file_info.st_size);

  multi_init(m);

  multi_setopt(m, CARLMOPT_SOCKETFUNCTION, carlSocketCallback);
  multi_setopt(m, CARLMOPT_SOCKETDATA, &sockets);

  multi_setopt(m, CARLMOPT_TIMERFUNCTION, carlTimerCallback);
  multi_setopt(m, CARLMOPT_TIMERDATA, &timeout);

  multi_add_handle(m, carl);

  while(!checkForCompletion(m, &success)) {
    fd_set readSet, writeSet;
    carl_socket_t maxFd = 0;
    struct timeval tv = {10, 0};

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    updateFdSet(&sockets.read, &readSet, &maxFd);
    updateFdSet(&sockets.write, &writeSet, &maxFd);

    if(timeout.tv_sec != -1) {
      int usTimeout = getMicroSecondTimeout(&timeout);
      tv.tv_sec = usTimeout / 1000000;
      tv.tv_usec = usTimeout % 1000000;
    }
    else if(maxFd <= 0) {
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
    }

    select_test((int)maxFd, &readSet, &writeSet, NULL, &tv);

    /* Check the sockets for reading / writing */
    checkFdSet(m, &sockets.read, &readSet, CARL_CSELECT_IN, "read");
    checkFdSet(m, &sockets.write, &writeSet, CARL_CSELECT_OUT, "write");

    if(timeout.tv_sec != -1 && getMicroSecondTimeout(&timeout) == 0) {
      /* Curl's timer has elapsed. */
      notifyCurl(m, CARL_SOCKET_TIMEOUT, 0, "timeout");
    }

    abort_on_test_timeout();
  }

  if(!success) {
    fprintf(stderr, "Error uploading file.\n");
    res = TEST_ERR_MAJOR_BAD;
  }

test_cleanup:

  /* proper cleanup sequence - type PB */

  carl_multi_remove_handle(m, carl);
  carl_easy_cleanup(carl);
  carl_multi_cleanup(m);
  carl_global_cleanup();

  /* close the local file */
  fclose(hd_src);

  /* free local memory */
  free(sockets.read.sockets);
  free(sockets.write.sockets);

  return res;
}
