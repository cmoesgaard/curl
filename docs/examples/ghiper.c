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
 * multi socket API usage together with with glib2
 * </DESC>
 */
/* Example application source code using the multi socket interface to
 * download many files at once.
 *
 * Written by Jeff Pohlmeyer

 Requires glib-2.x and a (POSIX?) system that has mkfifo().

 This is an adaptation of libcarl's "hipev.c" and libevent's "event-test.c"
 sample programs, adapted to use glib's g_io_channel in place of libevent.

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

 This is purely a demo app, all retrieved data is simply discarded by the write
 callback.

*/

#include <glib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <carl/carl.h>

#define MSG_OUT g_print   /* Change to "g_error" to write to stderr */
#define SHOW_VERBOSE 0    /* Set to non-zero for libcarl messages */
#define SHOW_PROGRESS 0   /* Set to non-zero to enable progress callback */

/* Global information, common to all connections */
typedef struct _GlobalInfo {
  CARLM *multi;
  guint timer_event;
  int still_running;
} GlobalInfo;

/* Information associated with a specific easy handle */
typedef struct _ConnInfo {
  CARL *easy;
  char *url;
  GlobalInfo *global;
  char error[CARL_ERROR_SIZE];
} ConnInfo;

/* Information associated with a specific socket */
typedef struct _SockInfo {
  carl_socket_t sockfd;
  CARL *easy;
  int action;
  long timeout;
  GIOChannel *ch;
  guint ev;
  GlobalInfo *global;
} SockInfo;

/* Die if we get a bad CARLMcode somewhere */
static void mcode_or_die(const char *where, CARLMcode code)
{
  if(CARLM_OK != code) {
    const char *s;
    switch(code) {
    case     CARLM_BAD_HANDLE:         s = "CARLM_BAD_HANDLE";         break;
    case     CARLM_BAD_EASY_HANDLE:    s = "CARLM_BAD_EASY_HANDLE";    break;
    case     CARLM_OUT_OF_MEMORY:      s = "CARLM_OUT_OF_MEMORY";      break;
    case     CARLM_INTERNAL_ERROR:     s = "CARLM_INTERNAL_ERROR";     break;
    case     CARLM_BAD_SOCKET:         s = "CARLM_BAD_SOCKET";         break;
    case     CARLM_UNKNOWN_OPTION:     s = "CARLM_UNKNOWN_OPTION";     break;
    case     CARLM_LAST:               s = "CARLM_LAST";               break;
    default: s = "CARLM_unknown";
    }
    MSG_OUT("ERROR: %s returns %s\n", where, s);
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

  MSG_OUT("REMAINING: %d\n", g->still_running);
  while((msg = carl_multi_info_read(g->multi, &msgs_left))) {
    if(msg->msg == CARLMSG_DONE) {
      easy = msg->easy_handle;
      res = msg->data.result;
      carl_easy_getinfo(easy, CARLINFO_PRIVATE, &conn);
      carl_easy_getinfo(easy, CARLINFO_EFFECTIVE_URL, &eff_url);
      MSG_OUT("DONE: %s => (%d) %s\n", eff_url, res, conn->error);
      carl_multi_remove_handle(g->multi, easy);
      free(conn->url);
      carl_easy_cleanup(easy);
      free(conn);
    }
  }
}

/* Called by glib when our timeout expires */
static gboolean timer_cb(gpointer data)
{
  GlobalInfo *g = (GlobalInfo *)data;
  CARLMcode rc;

  rc = carl_multi_socket_action(g->multi,
                                CARL_SOCKET_TIMEOUT, 0, &g->still_running);
  mcode_or_die("timer_cb: carl_multi_socket_action", rc);
  check_multi_info(g);
  return FALSE;
}

/* Update the event timer after carl_multi library calls */
static int update_timeout_cb(CARLM *multi, long timeout_ms, void *userp)
{
  struct timeval timeout;
  GlobalInfo *g = (GlobalInfo *)userp;
  timeout.tv_sec = timeout_ms/1000;
  timeout.tv_usec = (timeout_ms%1000)*1000;

  MSG_OUT("*** update_timeout_cb %ld => %ld:%ld ***\n",
          timeout_ms, timeout.tv_sec, timeout.tv_usec);

  /*
   * if timeout_ms is -1, just delete the timer
   *
   * For other values of timeout_ms, this should set or *update* the timer to
   * the new value
   */
  if(timeout_ms >= 0)
    g->timer_event = g_timeout_add(timeout_ms, timer_cb, g);
  return 0;
}

/* Called by glib when we get action on a multi socket */
static gboolean event_cb(GIOChannel *ch, GIOCondition condition, gpointer data)
{
  GlobalInfo *g = (GlobalInfo*) data;
  CARLMcode rc;
  int fd = g_io_channel_unix_get_fd(ch);

  int action =
    ((condition & G_IO_IN) ? CARL_CSELECT_IN : 0) |
    ((condition & G_IO_OUT) ? CARL_CSELECT_OUT : 0);

  rc = carl_multi_socket_action(g->multi, fd, action, &g->still_running);
  mcode_or_die("event_cb: carl_multi_socket_action", rc);

  check_multi_info(g);
  if(g->still_running) {
    return TRUE;
  }
  else {
    MSG_OUT("last transfer done, kill timeout\n");
    if(g->timer_event) {
      g_source_remove(g->timer_event);
    }
    return FALSE;
  }
}

/* Clean up the SockInfo structure */
static void remsock(SockInfo *f)
{
  if(!f) {
    return;
  }
  if(f->ev) {
    g_source_remove(f->ev);
  }
  g_free(f);
}

/* Assign information to a SockInfo structure */
static void setsock(SockInfo *f, carl_socket_t s, CARL *e, int act,
                    GlobalInfo *g)
{
  GIOCondition kind =
    ((act & CARL_POLL_IN) ? G_IO_IN : 0) |
    ((act & CARL_POLL_OUT) ? G_IO_OUT : 0);

  f->sockfd = s;
  f->action = act;
  f->easy = e;
  if(f->ev) {
    g_source_remove(f->ev);
  }
  f->ev = g_io_add_watch(f->ch, kind, event_cb, g);
}

/* Initialize a new SockInfo structure */
static void addsock(carl_socket_t s, CARL *easy, int action, GlobalInfo *g)
{
  SockInfo *fdp = g_malloc0(sizeof(SockInfo));

  fdp->global = g;
  fdp->ch = g_io_channel_unix_new(s);
  setsock(fdp, s, easy, action, g);
  carl_multi_assign(g->multi, s, fdp);
}

/* CARLMOPT_SOCKETFUNCTION */
static int sock_cb(CARL *e, carl_socket_t s, int what, void *cbp, void *sockp)
{
  GlobalInfo *g = (GlobalInfo*) cbp;
  SockInfo *fdp = (SockInfo*) sockp;
  static const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };

  MSG_OUT("socket callback: s=%d e=%p what=%s ", s, e, whatstr[what]);
  if(what == CARL_POLL_REMOVE) {
    MSG_OUT("\n");
    remsock(fdp);
  }
  else {
    if(!fdp) {
      MSG_OUT("Adding data: %s%s\n",
              (what & CARL_POLL_IN) ? "READ" : "",
              (what & CARL_POLL_OUT) ? "WRITE" : "");
      addsock(s, e, what, g);
    }
    else {
      MSG_OUT(
        "Changing action from %d to %d\n", fdp->action, what);
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
  MSG_OUT("Progress: %s (%g/%g)\n", conn->url, dlnow, dltotal);
  return 0;
}

/* Create a new easy handle, and add it to the global carl_multi */
static void new_conn(char *url, GlobalInfo *g)
{
  ConnInfo *conn;
  CARLMcode rc;

  conn = g_malloc0(sizeof(ConnInfo));
  conn->error[0]='\0';
  conn->easy = carl_easy_init();
  if(!conn->easy) {
    MSG_OUT("carl_easy_init() failed, exiting!\n");
    exit(2);
  }
  conn->global = g;
  conn->url = g_strdup(url);
  carl_easy_setopt(conn->easy, CARLOPT_URL, conn->url);
  carl_easy_setopt(conn->easy, CARLOPT_WRITEFUNCTION, write_cb);
  carl_easy_setopt(conn->easy, CARLOPT_WRITEDATA, &conn);
  carl_easy_setopt(conn->easy, CARLOPT_VERBOSE, (long)SHOW_VERBOSE);
  carl_easy_setopt(conn->easy, CARLOPT_ERRORBUFFER, conn->error);
  carl_easy_setopt(conn->easy, CARLOPT_PRIVATE, conn);
  carl_easy_setopt(conn->easy, CARLOPT_NOPROGRESS, SHOW_PROGRESS?0L:1L);
  carl_easy_setopt(conn->easy, CARLOPT_PROGRESSFUNCTION, prog_cb);
  carl_easy_setopt(conn->easy, CARLOPT_PROGRESSDATA, conn);
  carl_easy_setopt(conn->easy, CARLOPT_FOLLOWLOCATION, 1L);
  carl_easy_setopt(conn->easy, CARLOPT_CONNECTTIMEOUT, 30L);
  carl_easy_setopt(conn->easy, CARLOPT_LOW_SPEED_LIMIT, 1L);
  carl_easy_setopt(conn->easy, CARLOPT_LOW_SPEED_TIME, 30L);

  MSG_OUT("Adding easy %p to multi %p (%s)\n", conn->easy, g->multi, url);
  rc = carl_multi_add_handle(g->multi, conn->easy);
  mcode_or_die("new_conn: carl_multi_add_handle", rc);

  /* note that the add_handle() will set a time-out to trigger very soon so
     that the necessary socket_action() call will be called by this app */
}

/* This gets called by glib whenever data is received from the fifo */
static gboolean fifo_cb(GIOChannel *ch, GIOCondition condition, gpointer data)
{
#define BUF_SIZE 1024
  gsize len, tp;
  gchar *buf, *tmp, *all = NULL;
  GIOStatus rv;

  do {
    GError *err = NULL;
    rv = g_io_channel_read_line(ch, &buf, &len, &tp, &err);
    if(buf) {
      if(tp) {
        buf[tp]='\0';
      }
      new_conn(buf, (GlobalInfo*)data);
      g_free(buf);
    }
    else {
      buf = g_malloc(BUF_SIZE + 1);
      while(TRUE) {
        buf[BUF_SIZE]='\0';
        g_io_channel_read_chars(ch, buf, BUF_SIZE, &len, &err);
        if(len) {
          buf[len]='\0';
          if(all) {
            tmp = all;
            all = g_strdup_printf("%s%s", tmp, buf);
            g_free(tmp);
          }
          else {
            all = g_strdup(buf);
          }
        }
        else {
          break;
        }
      }
      if(all) {
        new_conn(all, (GlobalInfo*)data);
        g_free(all);
      }
      g_free(buf);
    }
    if(err) {
      g_error("fifo_cb: %s", err->message);
      g_free(err);
      break;
    }
  } while((len) && (rv == G_IO_STATUS_NORMAL));
  return TRUE;
}

int init_fifo(void)
{
  struct stat st;
  const char *fifo = "hiper.fifo";
  int socket;

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

  socket = open(fifo, O_RDWR | O_NONBLOCK, 0);

  if(socket == -1) {
    perror("open");
    exit(1);
  }
  MSG_OUT("Now, pipe some URL's into > %s\n", fifo);

  return socket;
}

int main(int argc, char **argv)
{
  GlobalInfo *g;
  GMainLoop*gmain;
  int fd;
  GIOChannel* ch;
  g = g_malloc0(sizeof(GlobalInfo));

  fd = init_fifo();
  ch = g_io_channel_unix_new(fd);
  g_io_add_watch(ch, G_IO_IN, fifo_cb, g);
  gmain = g_main_loop_new(NULL, FALSE);
  g->multi = carl_multi_init();
  carl_multi_setopt(g->multi, CARLMOPT_SOCKETFUNCTION, sock_cb);
  carl_multi_setopt(g->multi, CARLMOPT_SOCKETDATA, g);
  carl_multi_setopt(g->multi, CARLMOPT_TIMERFUNCTION, update_timeout_cb);
  carl_multi_setopt(g->multi, CARLMOPT_TIMERDATA, g);

  /* we don't call any carl_multi_socket*() function yet as we have no handles
     added! */

  g_main_loop_run(gmain);
  carl_multi_cleanup(g->multi);
  return 0;
}
