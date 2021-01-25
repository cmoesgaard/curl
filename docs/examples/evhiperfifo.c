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
 * multi socket interface together with libev
 * </DESC>
 */
/* Example application source code using the multi socket interface to
 * download many files at once.
 *
 * This example features the same basic functionality as hiperfifo.c does,
 * but this uses libev instead of libevent.
 *
 * Written by Jeff Pohlmeyer, converted to use libev by Markus Koetter

Requires libev and a (POSIX?) system that has mkfifo().

This is an adaptation of libcarl's "hipev.c" and libevent's "event-test.c"
sample programs.

When running, the program creates the named pipe "hiper.fifo"

Whenever there is input into the fifo, the program reads the input as a list
of URL's and creates some new easy handles to fetch each URL via the
carl_multi "hiper" API.


Thus, you can try a single URL:
  % echo http://www.yahoo.com > hiper.fifo

Or a whole bunch of them:
  % cat my-url-list > hiper.fifo

The fifo buffer is handled almost instantly, so you can even add more URL's
while the previous requests are still being downloaded.

Note:
  For the sake of simplicity, URL length is limited to 1023 char's !

This is purely a demo app, all retrieved data is simply discarded by the write
callback.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/poll.h>
#include <carl/carl.h>
#include <ev.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define DPRINT(x...) printf(x)

#define MSG_OUT stdout /* Send info to stdout, change to stderr if you want */


/* Global information, common to all connections */
typedef struct _GlobalInfo
{
  struct ev_loop *loop;
  struct ev_io fifo_event;
  struct ev_timer timer_event;
  CARLM *multi;
  int still_running;
  FILE *input;
} GlobalInfo;


/* Information associated with a specific easy handle */
typedef struct _ConnInfo
{
  CARL *easy;
  char *url;
  GlobalInfo *global;
  char error[CARL_ERROR_SIZE];
} ConnInfo;


/* Information associated with a specific socket */
typedef struct _SockInfo
{
  carl_socket_t sockfd;
  CARL *easy;
  int action;
  long timeout;
  struct ev_io ev;
  int evset;
  GlobalInfo *global;
} SockInfo;

static void timer_cb(EV_P_ struct ev_timer *w, int revents);

/* Update the event timer after carl_multi library calls */
static int multi_timer_cb(CARLM *multi, long timeout_ms, GlobalInfo *g)
{
  DPRINT("%s %li\n", __PRETTY_FUNCTION__,  timeout_ms);
  ev_timer_stop(g->loop, &g->timer_event);
  if(timeout_ms >= 0) {
    /* -1 means delete, other values are timeout times in milliseconds */
    double  t = timeout_ms / 1000;
    ev_timer_init(&g->timer_event, timer_cb, t, 0.);
    ev_timer_start(g->loop, &g->timer_event);
  }
  return 0;
}

/* Die if we get a bad CARLMcode somewhere */
static void mcode_or_die(const char *where, CARLMcode code)
{
  if(CARLM_OK != code) {
    const char *s;
    switch(code) {
    case CARLM_BAD_HANDLE:
      s = "CARLM_BAD_HANDLE";
      break;
    case CARLM_BAD_EASY_HANDLE:
      s = "CARLM_BAD_EASY_HANDLE";
      break;
    case CARLM_OUT_OF_MEMORY:
      s = "CARLM_OUT_OF_MEMORY";
      break;
    case CARLM_INTERNAL_ERROR:
      s = "CARLM_INTERNAL_ERROR";
      break;
    case CARLM_UNKNOWN_OPTION:
      s = "CARLM_UNKNOWN_OPTION";
      break;
    case CARLM_LAST:
      s = "CARLM_LAST";
      break;
    default:
      s = "CARLM_unknown";
      break;
    case CARLM_BAD_SOCKET:
      s = "CARLM_BAD_SOCKET";
      fprintf(MSG_OUT, "ERROR: %s returns %s\n", where, s);
      /* ignore this error */
      return;
    }
    fprintf(MSG_OUT, "ERROR: %s returns %s\n", where, s);
    exit(code);
  }
}



/* Check for completed transfers, and remove their easy handles */
static void check_multi_info(GlobalInfo *g)
{
  char *eff_url;
  CARLMsg *msg;
  int msgs_left;
  ConnInfo *conn;
  CARL *easy;
  CARLcode res;

  fprintf(MSG_OUT, "REMAINING: %d\n", g->still_running);
  while((msg = carl_multi_info_read(g->multi, &msgs_left))) {
    if(msg->msg == CARLMSG_DONE) {
      easy = msg->easy_handle;
      res = msg->data.result;
      carl_easy_getinfo(easy, CARLINFO_PRIVATE, &conn);
      carl_easy_getinfo(easy, CARLINFO_EFFECTIVE_URL, &eff_url);
      fprintf(MSG_OUT, "DONE: %s => (%d) %s\n", eff_url, res, conn->error);
      carl_multi_remove_handle(g->multi, easy);
      free(conn->url);
      carl_easy_cleanup(easy);
      free(conn);
    }
  }
}



/* Called by libevent when we get action on a multi socket */
static void event_cb(EV_P_ struct ev_io *w, int revents)
{
  DPRINT("%s  w %p revents %i\n", __PRETTY_FUNCTION__, w, revents);
  GlobalInfo *g = (GlobalInfo*) w->data;
  CARLMcode rc;

  int action = ((revents & EV_READ) ? CARL_POLL_IN : 0) |
    ((revents & EV_WRITE) ? CARL_POLL_OUT : 0);
  rc = carl_multi_socket_action(g->multi, w->fd, action, &g->still_running);
  mcode_or_die("event_cb: carl_multi_socket_action", rc);
  check_multi_info(g);
  if(g->still_running <= 0) {
    fprintf(MSG_OUT, "last transfer done, kill timeout\n");
    ev_timer_stop(g->loop, &g->timer_event);
  }
}

/* Called by libevent when our timeout expires */
static void timer_cb(EV_P_ struct ev_timer *w, int revents)
{
  DPRINT("%s  w %p revents %i\n", __PRETTY_FUNCTION__, w, revents);

  GlobalInfo *g = (GlobalInfo *)w->data;
  CARLMcode rc;

  rc = carl_multi_socket_action(g->multi, CARL_SOCKET_TIMEOUT, 0,
                                &g->still_running);
  mcode_or_die("timer_cb: carl_multi_socket_action", rc);
  check_multi_info(g);
}

/* Clean up the SockInfo structure */
static void remsock(SockInfo *f, GlobalInfo *g)
{
  printf("%s  \n", __PRETTY_FUNCTION__);
  if(f) {
    if(f->evset)
      ev_io_stop(g->loop, &f->ev);
    free(f);
  }
}



/* Assign information to a SockInfo structure */
static void setsock(SockInfo *f, carl_socket_t s, CARL *e, int act,
                    GlobalInfo *g)
{
  printf("%s  \n", __PRETTY_FUNCTION__);

  int kind = ((act & CARL_POLL_IN) ? EV_READ : 0) |
             ((act & CARL_POLL_OUT) ? EV_WRITE : 0);

  f->sockfd = s;
  f->action = act;
  f->easy = e;
  if(f->evset)
    ev_io_stop(g->loop, &f->ev);
  ev_io_init(&f->ev, event_cb, f->sockfd, kind);
  f->ev.data = g;
  f->evset = 1;
  ev_io_start(g->loop, &f->ev);
}



/* Initialize a new SockInfo structure */
static void addsock(carl_socket_t s, CARL *easy, int action, GlobalInfo *g)
{
  SockInfo *fdp = calloc(sizeof(SockInfo), 1);

  fdp->global = g;
  setsock(fdp, s, easy, action, g);
  carl_multi_assign(g->multi, s, fdp);
}

/* CARLMOPT_SOCKETFUNCTION */
static int sock_cb(CARL *e, carl_socket_t s, int what, void *cbp, void *sockp)
{
  DPRINT("%s e %p s %i what %i cbp %p sockp %p\n",
         __PRETTY_FUNCTION__, e, s, what, cbp, sockp);

  GlobalInfo *g = (GlobalInfo*) cbp;
  SockInfo *fdp = (SockInfo*) sockp;
  const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE"};

  fprintf(MSG_OUT,
          "socket callback: s=%d e=%p what=%s ", s, e, whatstr[what]);
  if(what == CARL_POLL_REMOVE) {
    fprintf(MSG_OUT, "\n");
    remsock(fdp, g);
  }
  else {
    if(!fdp) {
      fprintf(MSG_OUT, "Adding data: %s\n", whatstr[what]);
      addsock(s, e, what, g);
    }
    else {
      fprintf(MSG_OUT,
              "Changing action from %s to %s\n",
              whatstr[fdp->action], whatstr[what]);
      setsock(fdp, s, e, what, g);
    }
  }
  return 0;
}


/* CARLOPT_WRITEFUNCTION */
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  ConnInfo *conn = (ConnInfo*) data;
  (void)ptr;
  (void)conn;
  return realsize;
}


/* CARLOPT_PROGRESSFUNCTION */
static int prog_cb(void *p, double dltotal, double dlnow, double ult,
                   double uln)
{
  ConnInfo *conn = (ConnInfo *)p;
  (void)ult;
  (void)uln;

  fprintf(MSG_OUT, "Progress: %s (%g/%g)\n", conn->url, dlnow, dltotal);
  return 0;
}


/* Create a new easy handle, and add it to the global carl_multi */
static void new_conn(char *url, GlobalInfo *g)
{
  ConnInfo *conn;
  CARLMcode rc;

  conn = calloc(1, sizeof(ConnInfo));
  conn->error[0]='\0';

  conn->easy = carl_easy_init();
  if(!conn->easy) {
    fprintf(MSG_OUT, "carl_easy_init() failed, exiting!\n");
    exit(2);
  }
  conn->global = g;
  conn->url = strdup(url);
  carl_easy_setopt(conn->easy, CARLOPT_URL, conn->url);
  carl_easy_setopt(conn->easy, CARLOPT_WRITEFUNCTION, write_cb);
  carl_easy_setopt(conn->easy, CARLOPT_WRITEDATA, conn);
  carl_easy_setopt(conn->easy, CARLOPT_VERBOSE, 1L);
  carl_easy_setopt(conn->easy, CARLOPT_ERRORBUFFER, conn->error);
  carl_easy_setopt(conn->easy, CARLOPT_PRIVATE, conn);
  carl_easy_setopt(conn->easy, CARLOPT_NOPROGRESS, 0L);
  carl_easy_setopt(conn->easy, CARLOPT_PROGRESSFUNCTION, prog_cb);
  carl_easy_setopt(conn->easy, CARLOPT_PROGRESSDATA, conn);
  carl_easy_setopt(conn->easy, CARLOPT_LOW_SPEED_TIME, 3L);
  carl_easy_setopt(conn->easy, CARLOPT_LOW_SPEED_LIMIT, 10L);

  fprintf(MSG_OUT,
          "Adding easy %p to multi %p (%s)\n", conn->easy, g->multi, url);
  rc = carl_multi_add_handle(g->multi, conn->easy);
  mcode_or_die("new_conn: carl_multi_add_handle", rc);

  /* note that the add_handle() will set a time-out to trigger very soon so
     that the necessary socket_action() call will be called by this app */
}

/* This gets called whenever data is received from the fifo */
static void fifo_cb(EV_P_ struct ev_io *w, int revents)
{
  char s[1024];
  long int rv = 0;
  int n = 0;
  GlobalInfo *g = (GlobalInfo *)w->data;

  do {
    s[0]='\0';
    rv = fscanf(g->input, "%1023s%n", s, &n);
    s[n]='\0';
    if(n && s[0]) {
      new_conn(s, g);  /* if we read a URL, go get it! */
    }
    else
      break;
  } while(rv != EOF);
}

/* Create a named pipe and tell libevent to monitor it */
static int init_fifo(GlobalInfo *g)
{
  struct stat st;
  static const char *fifo = "hiper.fifo";
  carl_socket_t sockfd;

  fprintf(MSG_OUT, "Creating named pipe \"%s\"\n", fifo);
  if(lstat (fifo, &st) == 0) {
    if((st.st_mode & S_IFMT) == S_IFREG) {
      errno = EEXIST;
      perror("lstat");
      exit(1);
    }
  }
  unlink(fifo);
  if(mkfifo (fifo, 0600) == -1) {
    perror("mkfifo");
    exit(1);
  }
  sockfd = open(fifo, O_RDWR | O_NONBLOCK, 0);
  if(sockfd == -1) {
    perror("open");
    exit(1);
  }
  g->input = fdopen(sockfd, "r");

  fprintf(MSG_OUT, "Now, pipe some URL's into > %s\n", fifo);
  ev_io_init(&g->fifo_event, fifo_cb, sockfd, EV_READ);
  ev_io_start(g->loop, &g->fifo_event);
  return (0);
}

int main(int argc, char **argv)
{
  GlobalInfo g;
  (void)argc;
  (void)argv;

  memset(&g, 0, sizeof(GlobalInfo));
  g.loop = ev_default_loop(0);

  init_fifo(&g);
  g.multi = carl_multi_init();

  ev_timer_init(&g.timer_event, timer_cb, 0., 0.);
  g.timer_event.data = &g;
  g.fifo_event.data = &g;
  carl_multi_setopt(g.multi, CARLMOPT_SOCKETFUNCTION, sock_cb);
  carl_multi_setopt(g.multi, CARLMOPT_SOCKETDATA, &g);
  carl_multi_setopt(g.multi, CARLMOPT_TIMERFUNCTION, multi_timer_cb);
  carl_multi_setopt(g.multi, CARLMOPT_TIMERDATA, &g);

  /* we don't call any carl_multi_socket*() function yet as we have no handles
     added! */

  ev_loop(g.loop, 0);
  carl_multi_cleanup(g.multi);
  return 0;
}
