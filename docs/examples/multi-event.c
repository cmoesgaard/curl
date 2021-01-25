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
 * multi_socket API using libevent
 * </DESC>
 */

#include <stdio.h>
#include <stdlib.h>
#include <event2/event.h>
#include <carl/carl.h>

struct event_base *base;
CARLM *carl_handle;
struct event *timeout;

typedef struct carl_context_s {
  struct event *event;
  carl_socket_t sockfd;
} carl_context_t;

static void carl_perform(int fd, short event, void *arg);

static carl_context_t *create_carl_context(carl_socket_t sockfd)
{
  carl_context_t *context;

  context = (carl_context_t *) malloc(sizeof(*context));

  context->sockfd = sockfd;

  context->event = event_new(base, sockfd, 0, carl_perform, context);

  return context;
}

static void destroy_carl_context(carl_context_t *context)
{
  event_del(context->event);
  event_free(context->event);
  free(context);
}

static void add_download(const char *url, int num)
{
  char filename[50];
  FILE *file;
  CARL *handle;

  snprintf(filename, 50, "%d.download", num);

  file = fopen(filename, "wb");
  if(!file) {
    fprintf(stderr, "Error opening %s\n", filename);
    return;
  }

  handle = carl_easy_init();
  carl_easy_setopt(handle, CARLOPT_WRITEDATA, file);
  carl_easy_setopt(handle, CARLOPT_PRIVATE, file);
  carl_easy_setopt(handle, CARLOPT_URL, url);
  carl_multi_add_handle(carl_handle, handle);
  fprintf(stderr, "Added download %s -> %s\n", url, filename);
}

static void check_multi_info(void)
{
  char *done_url;
  CARLMsg *message;
  int pending;
  CARL *easy_handle;
  FILE *file;

  while((message = carl_multi_info_read(carl_handle, &pending))) {
    switch(message->msg) {
    case CARLMSG_DONE:
      /* Do not use message data after calling carl_multi_remove_handle() and
         carl_easy_cleanup(). As per carl_multi_info_read() docs:
         "WARNING: The data the returned pointer points to will not survive
         calling carl_multi_cleanup, carl_multi_remove_handle or
         carl_easy_cleanup." */
      easy_handle = message->easy_handle;

      carl_easy_getinfo(easy_handle, CARLINFO_EFFECTIVE_URL, &done_url);
      carl_easy_getinfo(easy_handle, CARLINFO_PRIVATE, &file);
      printf("%s DONE\n", done_url);

      carl_multi_remove_handle(carl_handle, easy_handle);
      carl_easy_cleanup(easy_handle);
      if(file) {
        fclose(file);
      }
      break;

    default:
      fprintf(stderr, "CARLMSG default\n");
      break;
    }
  }
}

static void carl_perform(int fd, short event, void *arg)
{
  int running_handles;
  int flags = 0;
  carl_context_t *context;

  if(event & EV_READ)
    flags |= CARL_CSELECT_IN;
  if(event & EV_WRITE)
    flags |= CARL_CSELECT_OUT;

  context = (carl_context_t *) arg;

  carl_multi_socket_action(carl_handle, context->sockfd, flags,
                           &running_handles);

  check_multi_info();
}

static void on_timeout(evutil_socket_t fd, short events, void *arg)
{
  int running_handles;
  carl_multi_socket_action(carl_handle, CARL_SOCKET_TIMEOUT, 0,
                           &running_handles);
  check_multi_info();
}

static int start_timeout(CARLM *multi, long timeout_ms, void *userp)
{
  if(timeout_ms < 0) {
    evtimer_del(timeout);
  }
  else {
    if(timeout_ms == 0)
      timeout_ms = 1; /* 0 means directly call socket_action, but we'll do it
                         in a bit */
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    evtimer_del(timeout);
    evtimer_add(timeout, &tv);
  }
  return 0;
}

static int handle_socket(CARL *easy, carl_socket_t s, int action, void *userp,
                  void *socketp)
{
  carl_context_t *carl_context;
  int events = 0;

  switch(action) {
  case CARL_POLL_IN:
  case CARL_POLL_OUT:
  case CARL_POLL_INOUT:
    carl_context = socketp ?
      (carl_context_t *) socketp : create_carl_context(s);

    carl_multi_assign(carl_handle, s, (void *) carl_context);

    if(action != CARL_POLL_IN)
      events |= EV_WRITE;
    if(action != CARL_POLL_OUT)
      events |= EV_READ;

    events |= EV_PERSIST;

    event_del(carl_context->event);
    event_assign(carl_context->event, base, carl_context->sockfd, events,
      carl_perform, carl_context);
    event_add(carl_context->event, NULL);

    break;
  case CARL_POLL_REMOVE:
    if(socketp) {
      event_del(((carl_context_t*) socketp)->event);
      destroy_carl_context((carl_context_t*) socketp);
      carl_multi_assign(carl_handle, s, NULL);
    }
    break;
  default:
    abort();
  }

  return 0;
}

int main(int argc, char **argv)
{
  if(argc <= 1)
    return 0;

  if(carl_global_init(CARL_GLOBAL_ALL)) {
    fprintf(stderr, "Could not init carl\n");
    return 1;
  }

  base = event_base_new();
  timeout = evtimer_new(base, on_timeout, NULL);

  carl_handle = carl_multi_init();
  carl_multi_setopt(carl_handle, CARLMOPT_SOCKETFUNCTION, handle_socket);
  carl_multi_setopt(carl_handle, CARLMOPT_TIMERFUNCTION, start_timeout);

  while(argc-- > 1) {
    add_download(argv[argc], argc);
  }

  event_base_dispatch(base);

  carl_multi_cleanup(carl_handle);
  event_free(timeout);
  event_base_free(base);

  libevent_global_shutdown();
  carl_global_cleanup();

  return 0;
}
