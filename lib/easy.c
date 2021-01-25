/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#include "carl_setup.h"

/*
 * See comment in carl_memory.h for the explanation of this sanity check.
 */

#ifdef CARLX_NO_MEMORY_CALLBACKS
#error "libcarl shall not ever be built with CARLX_NO_MEMORY_CALLBACKS defined"
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "urldata.h"
#include <carl/carl.h>
#include "transfer.h"
#include "vtls/vtls.h"
#include "url.h"
#include "getinfo.h"
#include "hostip.h"
#include "share.h"
#include "strdup.h"
#include "progress.h"
#include "easyif.h"
#include "multiif.h"
#include "select.h"
#include "sendf.h" /* for failf function prototype */
#include "connect.h" /* for Curl_getconnectinfo */
#include "slist.h"
#include "mime.h"
#include "amigaos.h"
#include "non-ascii.h"
#include "warnless.h"
#include "multiif.h"
#include "sigpipe.h"
#include "vssh/ssh.h"
#include "setopt.h"
#include "http_digest.h"
#include "system_win32.h"
#include "http2.h"
#include "dynbuf.h"
#include "altsvc.h"
#include "hsts.h"

/* The last 3 #include files should be in this order */
#include "carl_printf.h"
#include "carl_memory.h"
#include "memdebug.h"

/* true globals -- for carl_global_init() and carl_global_cleanup() */
static unsigned int  initialized;
static long          init_flags;

/*
 * strdup (and other memory functions) is redefined in complicated
 * ways, but at this point it must be defined as the system-supplied strdup
 * so the callback pointer is initialized correctly.
 */
#if defined(_WIN32_WCE)
#define system_strdup _strdup
#elif !defined(HAVE_STRDUP)
#define system_strdup carlx_strdup
#else
#define system_strdup strdup
#endif

#if defined(_MSC_VER) && defined(_DLL) && !defined(__POCC__)
#  pragma warning(disable:4232) /* MSVC extension, dllimport identity */
#endif

/*
 * If a memory-using function (like carl_getenv) is used before
 * carl_global_init() is called, we need to have these pointers set already.
 */
carl_malloc_callback Curl_cmalloc = (carl_malloc_callback)malloc;
carl_free_callback Curl_cfree = (carl_free_callback)free;
carl_realloc_callback Curl_crealloc = (carl_realloc_callback)realloc;
carl_strdup_callback Curl_cstrdup = (carl_strdup_callback)system_strdup;
carl_calloc_callback Curl_ccalloc = (carl_calloc_callback)calloc;
#if defined(WIN32) && defined(UNICODE)
carl_wcsdup_callback Curl_cwcsdup = (carl_wcsdup_callback)_wcsdup;
#endif

#if defined(_MSC_VER) && defined(_DLL) && !defined(__POCC__)
#  pragma warning(default:4232) /* MSVC extension, dllimport identity */
#endif

#ifdef DEBUGBUILD
static char *leakpointer;
#endif

/**
 * carl_global_init() globally initializes carl given a bitwise set of the
 * different features of what to initialize.
 */
static CARLcode global_init(long flags, bool memoryfuncs)
{
  if(initialized++)
    return CARLE_OK;

  if(memoryfuncs) {
    /* Setup the default memory functions here (again) */
    Curl_cmalloc = (carl_malloc_callback)malloc;
    Curl_cfree = (carl_free_callback)free;
    Curl_crealloc = (carl_realloc_callback)realloc;
    Curl_cstrdup = (carl_strdup_callback)system_strdup;
    Curl_ccalloc = (carl_calloc_callback)calloc;
#if defined(WIN32) && defined(UNICODE)
    Curl_cwcsdup = (carl_wcsdup_callback)_wcsdup;
#endif
  }

  if(!Curl_ssl_init()) {
    DEBUGF(fprintf(stderr, "Error: Curl_ssl_init failed\n"));
    goto fail;
  }

#ifdef WIN32
  if(Curl_win32_init(flags)) {
    DEBUGF(fprintf(stderr, "Error: win32_init failed\n"));
    goto fail;
  }
#endif

#ifdef __AMIGA__
  if(!Curl_amiga_init()) {
    DEBUGF(fprintf(stderr, "Error: Curl_amiga_init failed\n"));
    goto fail;
  }
#endif

#ifdef NETWARE
  if(netware_init()) {
    DEBUGF(fprintf(stderr, "Warning: LONG namespace not available\n"));
  }
#endif

  if(Curl_resolver_global_init()) {
    DEBUGF(fprintf(stderr, "Error: resolver_global_init failed\n"));
    goto fail;
  }

#if defined(USE_SSH)
  if(Curl_ssh_init()) {
    goto fail;
  }
#endif

#ifdef USE_WOLFSSH
  if(WS_SUCCESS != wolfSSH_Init()) {
    DEBUGF(fprintf(stderr, "Error: wolfSSH_Init failed\n"));
    return CARLE_FAILED_INIT;
  }
#endif

  init_flags = flags;

#ifdef DEBUGBUILD
  if(getenv("CARL_GLOBAL_INIT"))
    /* alloc data that will leak if *cleanup() is not called! */
    leakpointer = malloc(1);
#endif

  return CARLE_OK;

  fail:
  initialized--; /* undo the increase */
  return CARLE_FAILED_INIT;
}


/**
 * carl_global_init() globally initializes carl given a bitwise set of the
 * different features of what to initialize.
 */
CARLcode carl_global_init(long flags)
{
  return global_init(flags, TRUE);
}

/*
 * carl_global_init_mem() globally initializes carl and also registers the
 * user provided callback routines.
 */
CARLcode carl_global_init_mem(long flags, carl_malloc_callback m,
                              carl_free_callback f, carl_realloc_callback r,
                              carl_strdup_callback s, carl_calloc_callback c)
{
  /* Invalid input, return immediately */
  if(!m || !f || !r || !s || !c)
    return CARLE_FAILED_INIT;

  if(initialized) {
    /* Already initialized, don't do it again, but bump the variable anyway to
       work like carl_global_init() and require the same amount of cleanup
       calls. */
    initialized++;
    return CARLE_OK;
  }

  /* set memory functions before global_init() in case it wants memory
     functions */
  Curl_cmalloc = m;
  Curl_cfree = f;
  Curl_cstrdup = s;
  Curl_crealloc = r;
  Curl_ccalloc = c;

  /* Call the actual init function, but without setting */
  return global_init(flags, FALSE);
}

/**
 * carl_global_cleanup() globally cleanups carl, uses the value of
 * "init_flags" to determine what needs to be cleaned up and what doesn't.
 */
void carl_global_cleanup(void)
{
  if(!initialized)
    return;

  if(--initialized)
    return;

  Curl_ssl_cleanup();
  Curl_resolver_global_cleanup();

#ifdef WIN32
  Curl_win32_cleanup(init_flags);
#endif

  Curl_amiga_cleanup();

  Curl_ssh_cleanup();

#ifdef USE_WOLFSSH
  (void)wolfSSH_Cleanup();
#endif
#ifdef DEBUGBUILD
  free(leakpointer);
#endif

  init_flags  = 0;
}

/*
 * carl_easy_init() is the external interface to alloc, setup and init an
 * easy handle that is returned. If anything goes wrong, NULL is returned.
 */
struct Curl_easy *carl_easy_init(void)
{
  CARLcode result;
  struct Curl_easy *data;

  /* Make sure we inited the global SSL stuff */
  if(!initialized) {
    result = carl_global_init(CARL_GLOBAL_DEFAULT);
    if(result) {
      /* something in the global init failed, return nothing */
      DEBUGF(fprintf(stderr, "Error: carl_global_init failed\n"));
      return NULL;
    }
  }

  /* We use carl_open() with undefined URL so far */
  result = Curl_open(&data);
  if(result) {
    DEBUGF(fprintf(stderr, "Error: Curl_open failed\n"));
    return NULL;
  }

  return data;
}

#ifdef CARLDEBUG

struct socketmonitor {
  struct socketmonitor *next; /* the next node in the list or NULL */
  struct pollfd socket; /* socket info of what to monitor */
};

struct events {
  long ms;              /* timeout, run the timeout function when reached */
  bool msbump;          /* set TRUE when timeout is set by callback */
  int num_sockets;      /* number of nodes in the monitor list */
  struct socketmonitor *list; /* list of sockets to monitor */
  int running_handles;  /* store the returned number */
};

/* events_timer
 *
 * Callback that gets called with a new value when the timeout should be
 * updated.
 */

static int events_timer(struct Curl_multi *multi,    /* multi handle */
                        long timeout_ms, /* see above */
                        void *userp)    /* private callback pointer */
{
  struct events *ev = userp;
  (void)multi;
  if(timeout_ms == -1)
    /* timeout removed */
    timeout_ms = 0;
  else if(timeout_ms == 0)
    /* timeout is already reached! */
    timeout_ms = 1; /* trigger asap */

  ev->ms = timeout_ms;
  ev->msbump = TRUE;
  return 0;
}


/* poll2cselect
 *
 * convert from poll() bit definitions to libcarl's CARL_CSELECT_* ones
 */
static int poll2cselect(int pollmask)
{
  int omask = 0;
  if(pollmask & POLLIN)
    omask |= CARL_CSELECT_IN;
  if(pollmask & POLLOUT)
    omask |= CARL_CSELECT_OUT;
  if(pollmask & POLLERR)
    omask |= CARL_CSELECT_ERR;
  return omask;
}


/* socketcb2poll
 *
 * convert from libcarl' CARL_POLL_* bit definitions to poll()'s
 */
static short socketcb2poll(int pollmask)
{
  short omask = 0;
  if(pollmask & CARL_POLL_IN)
    omask |= POLLIN;
  if(pollmask & CARL_POLL_OUT)
    omask |= POLLOUT;
  return omask;
}

/* events_socket
 *
 * Callback that gets called with information about socket activity to
 * monitor.
 */
static int events_socket(struct Curl_easy *easy,      /* easy handle */
                         carl_socket_t s, /* socket */
                         int what,        /* see above */
                         void *userp,     /* private callback
                                             pointer */
                         void *socketp)   /* private socket
                                             pointer */
{
  struct events *ev = userp;
  struct socketmonitor *m;
  struct socketmonitor *prev = NULL;

#if defined(CARL_DISABLE_VERBOSE_STRINGS)
  (void) easy;
#endif
  (void)socketp;

  m = ev->list;
  while(m) {
    if(m->socket.fd == s) {

      if(what == CARL_POLL_REMOVE) {
        struct socketmonitor *nxt = m->next;
        /* remove this node from the list of monitored sockets */
        if(prev)
          prev->next = nxt;
        else
          ev->list = nxt;
        free(m);
        m = nxt;
        infof(easy, "socket cb: socket %d REMOVED\n", s);
      }
      else {
        /* The socket 's' is already being monitored, update the activity
           mask. Convert from libcarl bitmask to the poll one. */
        m->socket.events = socketcb2poll(what);
        infof(easy, "socket cb: socket %d UPDATED as %s%s\n", s,
              (what&CARL_POLL_IN)?"IN":"",
              (what&CARL_POLL_OUT)?"OUT":"");
      }
      break;
    }
    prev = m;
    m = m->next; /* move to next node */
  }
  if(!m) {
    if(what == CARL_POLL_REMOVE) {
      /* this happens a bit too often, libcarl fix perhaps? */
      /* fprintf(stderr,
         "%s: socket %d asked to be REMOVED but not present!\n",
                 __func__, s); */
    }
    else {
      m = malloc(sizeof(struct socketmonitor));
      if(m) {
        m->next = ev->list;
        m->socket.fd = s;
        m->socket.events = socketcb2poll(what);
        m->socket.revents = 0;
        ev->list = m;
        infof(easy, "socket cb: socket %d ADDED as %s%s\n", s,
              (what&CARL_POLL_IN)?"IN":"",
              (what&CARL_POLL_OUT)?"OUT":"");
      }
      else
        return CARLE_OUT_OF_MEMORY;
    }
  }

  return 0;
}


/*
 * events_setup()
 *
 * Do the multi handle setups that only event-based transfers need.
 */
static void events_setup(struct Curl_multi *multi, struct events *ev)
{
  /* timer callback */
  carl_multi_setopt(multi, CARLMOPT_TIMERFUNCTION, events_timer);
  carl_multi_setopt(multi, CARLMOPT_TIMERDATA, ev);

  /* socket callback */
  carl_multi_setopt(multi, CARLMOPT_SOCKETFUNCTION, events_socket);
  carl_multi_setopt(multi, CARLMOPT_SOCKETDATA, ev);
}


/* wait_or_timeout()
 *
 * waits for activity on any of the given sockets, or the timeout to trigger.
 */

static CARLcode wait_or_timeout(struct Curl_multi *multi, struct events *ev)
{
  bool done = FALSE;
  CARLMcode mcode = CARLM_OK;
  CARLcode result = CARLE_OK;

  while(!done) {
    CARLMsg *msg;
    struct socketmonitor *m;
    struct pollfd *f;
    struct pollfd fds[4];
    int numfds = 0;
    int pollrc;
    int i;
    struct carltime before;
    struct carltime after;

    /* populate the fds[] array */
    for(m = ev->list, f = &fds[0]; m; m = m->next) {
      f->fd = m->socket.fd;
      f->events = m->socket.events;
      f->revents = 0;
      /* fprintf(stderr, "poll() %d check socket %d\n", numfds, f->fd); */
      f++;
      numfds++;
    }

    /* get the time stamp to use to figure out how long poll takes */
    before = Curl_now();

    /* wait for activity or timeout */
    pollrc = Curl_poll(fds, numfds, ev->ms);

    after = Curl_now();

    ev->msbump = FALSE; /* reset here */

    if(0 == pollrc) {
      /* timeout! */
      ev->ms = 0;
      /* fprintf(stderr, "call carl_multi_socket_action(TIMEOUT)\n"); */
      mcode = carl_multi_socket_action(multi, CARL_SOCKET_TIMEOUT, 0,
                                       &ev->running_handles);
    }
    else if(pollrc > 0) {
      /* loop over the monitored sockets to see which ones had activity */
      for(i = 0; i< numfds; i++) {
        if(fds[i].revents) {
          /* socket activity, tell libcarl */
          int act = poll2cselect(fds[i].revents); /* convert */
          infof(multi->easyp, "call carl_multi_socket_action(socket %d)\n",
                fds[i].fd);
          mcode = carl_multi_socket_action(multi, fds[i].fd, act,
                                           &ev->running_handles);
        }
      }

      if(!ev->msbump) {
        /* If nothing updated the timeout, we decrease it by the spent time.
         * If it was updated, it has the new timeout time stored already.
         */
        timediff_t timediff = Curl_timediff(after, before);
        if(timediff > 0) {
          if(timediff > ev->ms)
            ev->ms = 0;
          else
            ev->ms -= (long)timediff;
        }
      }
    }
    else
      return CARLE_RECV_ERROR;

    if(mcode)
      return CARLE_URL_MALFORMAT;

    /* we don't really care about the "msgs_in_queue" value returned in the
       second argument */
    msg = carl_multi_info_read(multi, &pollrc);
    if(msg) {
      result = msg->data.result;
      done = TRUE;
    }
  }

  return result;
}


/* easy_events()
 *
 * Runs a transfer in a blocking manner using the events-based API
 */
static CARLcode easy_events(struct Curl_multi *multi)
{
  /* this struct is made static to allow it to be used after this function
     returns and carl_multi_remove_handle() is called */
  static struct events evs = {2, FALSE, 0, NULL, 0};

  /* if running event-based, do some further multi inits */
  events_setup(multi, &evs);

  return wait_or_timeout(multi, &evs);
}
#else /* CARLDEBUG */
/* when not built with debug, this function doesn't exist */
#define easy_events(x) CARLE_NOT_BUILT_IN
#endif

static CARLcode easy_transfer(struct Curl_multi *multi)
{
  bool done = FALSE;
  CARLMcode mcode = CARLM_OK;
  CARLcode result = CARLE_OK;

  while(!done && !mcode) {
    int still_running = 0;

    mcode = carl_multi_poll(multi, NULL, 0, 1000, NULL);

    if(!mcode)
      mcode = carl_multi_perform(multi, &still_running);

    /* only read 'still_running' if carl_multi_perform() return OK */
    if(!mcode && !still_running) {
      int rc;
      CARLMsg *msg = carl_multi_info_read(multi, &rc);
      if(msg) {
        result = msg->data.result;
        done = TRUE;
      }
    }
  }

  /* Make sure to return some kind of error if there was a multi problem */
  if(mcode) {
    result = (mcode == CARLM_OUT_OF_MEMORY) ? CARLE_OUT_OF_MEMORY :
              /* The other multi errors should never happen, so return
                 something suitably generic */
              CARLE_BAD_FUNCTION_ARGUMENT;
  }

  return result;
}


/*
 * easy_perform() is the external interface that performs a blocking
 * transfer as previously setup.
 *
 * CONCEPT: This function creates a multi handle, adds the easy handle to it,
 * runs carl_multi_perform() until the transfer is done, then detaches the
 * easy handle, destroys the multi handle and returns the easy handle's return
 * code.
 *
 * REALITY: it can't just create and destroy the multi handle that easily. It
 * needs to keep it around since if this easy handle is used again by this
 * function, the same multi handle must be re-used so that the same pools and
 * caches can be used.
 *
 * DEBUG: if 'events' is set TRUE, this function will use a replacement engine
 * instead of carl_multi_perform() and use carl_multi_socket_action().
 */
static CARLcode easy_perform(struct Curl_easy *data, bool events)
{
  struct Curl_multi *multi;
  CARLMcode mcode;
  CARLcode result = CARLE_OK;
  SIGPIPE_VARIABLE(pipe_st);

  if(!data)
    return CARLE_BAD_FUNCTION_ARGUMENT;

  if(data->set.errorbuffer)
    /* clear this as early as possible */
    data->set.errorbuffer[0] = 0;

  if(data->multi) {
    failf(data, "easy handle already used in multi handle");
    return CARLE_FAILED_INIT;
  }

  if(data->multi_easy)
    multi = data->multi_easy;
  else {
    /* this multi handle will only ever have a single easy handled attached
       to it, so make it use minimal hashes */
    multi = Curl_multi_handle(1, 3);
    if(!multi)
      return CARLE_OUT_OF_MEMORY;
    data->multi_easy = multi;
  }

  if(multi->in_callback)
    return CARLE_RECURSIVE_API_CALL;

  /* Copy the MAXCONNECTS option to the multi handle */
  carl_multi_setopt(multi, CARLMOPT_MAXCONNECTS, data->set.maxconnects);

  mcode = carl_multi_add_handle(multi, data);
  if(mcode) {
    carl_multi_cleanup(multi);
    data->multi_easy = NULL;
    if(mcode == CARLM_OUT_OF_MEMORY)
      return CARLE_OUT_OF_MEMORY;
    return CARLE_FAILED_INIT;
  }

  sigpipe_ignore(data, &pipe_st);

  /* run the transfer */
  result = events ? easy_events(multi) : easy_transfer(multi);

  /* ignoring the return code isn't nice, but atm we can't really handle
     a failure here, room for future improvement! */
  (void)carl_multi_remove_handle(multi, data);

  sigpipe_restore(&pipe_st);

  /* The multi handle is kept alive, owned by the easy handle */
  return result;
}


/*
 * carl_easy_perform() is the external interface that performs a blocking
 * transfer as previously setup.
 */
CARLcode carl_easy_perform(struct Curl_easy *data)
{
  return easy_perform(data, FALSE);
}

#ifdef CARLDEBUG
/*
 * carl_easy_perform_ev() is the external interface that performs a blocking
 * transfer using the event-based API internally.
 */
CARLcode carl_easy_perform_ev(struct Curl_easy *data)
{
  return easy_perform(data, TRUE);
}

#endif

/*
 * carl_easy_cleanup() is the external interface to cleaning/freeing the given
 * easy handle.
 */
void carl_easy_cleanup(struct Curl_easy *data)
{
  SIGPIPE_VARIABLE(pipe_st);

  if(!data)
    return;

  sigpipe_ignore(data, &pipe_st);
  Curl_close(&data);
  sigpipe_restore(&pipe_st);
}

/*
 * carl_easy_getinfo() is an external interface that allows an app to retrieve
 * information from a performed transfer and similar.
 */
#undef carl_easy_getinfo
CARLcode carl_easy_getinfo(struct Curl_easy *data, CARLINFO info, ...)
{
  va_list arg;
  void *paramp;
  CARLcode result;

  va_start(arg, info);
  paramp = va_arg(arg, void *);

  result = Curl_getinfo(data, info, paramp);

  va_end(arg);
  return result;
}

static CARLcode dupset(struct Curl_easy *dst, struct Curl_easy *src)
{
  CARLcode result = CARLE_OK;
  enum dupstring i;
  enum dupblob j;

  /* Copy src->set into dst->set first, then deal with the strings
     afterwards */
  dst->set = src->set;
  Curl_mime_initpart(&dst->set.mimepost, dst);

  /* clear all string pointers first */
  memset(dst->set.str, 0, STRING_LAST * sizeof(char *));

  /* duplicate all strings */
  for(i = (enum dupstring)0; i< STRING_LASTZEROTERMINATED; i++) {
    result = Curl_setstropt(&dst->set.str[i], src->set.str[i]);
    if(result)
      return result;
  }

  /* clear all blob pointers first */
  memset(dst->set.blobs, 0, BLOB_LAST * sizeof(struct carl_blob *));
  /* duplicate all blobs */
  for(j = (enum dupblob)0; j < BLOB_LAST; j++) {
    result = Curl_setblobopt(&dst->set.blobs[j], src->set.blobs[j]);
    /* Curl_setstropt return CARLE_BAD_FUNCTION_ARGUMENT with blob */
    if(result)
      return result;
  }

  /* duplicate memory areas pointed to */
  i = STRING_COPYPOSTFIELDS;
  if(src->set.postfieldsize && src->set.str[i]) {
    /* postfieldsize is carl_off_t, Curl_memdup() takes a size_t ... */
    dst->set.str[i] = Curl_memdup(src->set.str[i],
                                  carlx_sotouz(src->set.postfieldsize));
    if(!dst->set.str[i])
      return CARLE_OUT_OF_MEMORY;
    /* point to the new copy */
    dst->set.postfields = dst->set.str[i];
  }

  /* Duplicate mime data. */
  result = Curl_mime_duppart(&dst->set.mimepost, &src->set.mimepost);

  if(src->set.resolve)
    dst->change.resolve = dst->set.resolve;

  return result;
}

/*
 * carl_easy_duphandle() is an external interface to allow duplication of a
 * given input easy handle. The returned handle will be a new working handle
 * with all options set exactly as the input source handle.
 */
struct Curl_easy *carl_easy_duphandle(struct Curl_easy *data)
{
  struct Curl_easy *outcarl = calloc(1, sizeof(struct Curl_easy));
  if(NULL == outcarl)
    goto fail;

  /*
   * We setup a few buffers we need. We should probably make them
   * get setup on-demand in the code, as that would probably decrease
   * the likeliness of us forgetting to init a buffer here in the future.
   */
  outcarl->set.buffer_size = data->set.buffer_size;

  /* copy all userdefined values */
  if(dupset(outcarl, data))
    goto fail;

  Curl_dyn_init(&outcarl->state.headerb, CARL_MAX_HTTP_HEADER);

  /* the connection cache is setup on demand */
  outcarl->state.conn_cache = NULL;
  outcarl->state.lastconnect_id = -1;

  outcarl->progress.flags    = data->progress.flags;
  outcarl->progress.callback = data->progress.callback;

  if(data->cookies) {
    /* If cookies are enabled in the parent handle, we enable them
       in the clone as well! */
    outcarl->cookies = Curl_cookie_init(data,
                                        data->cookies->filename,
                                        outcarl->cookies,
                                        data->set.cookiesession);
    if(!outcarl->cookies)
      goto fail;
  }

  /* duplicate all values in 'change' */
  if(data->change.cookielist) {
    outcarl->change.cookielist =
      Curl_slist_duplicate(data->change.cookielist);
    if(!outcarl->change.cookielist)
      goto fail;
  }

  if(data->change.url) {
    outcarl->change.url = strdup(data->change.url);
    if(!outcarl->change.url)
      goto fail;
    outcarl->change.url_alloc = TRUE;
  }

  if(data->change.referer) {
    outcarl->change.referer = strdup(data->change.referer);
    if(!outcarl->change.referer)
      goto fail;
    outcarl->change.referer_alloc = TRUE;
  }

  /* Reinitialize an SSL engine for the new handle
   * note: the engine name has already been copied by dupset */
  if(outcarl->set.str[STRING_SSL_ENGINE]) {
    if(Curl_ssl_set_engine(outcarl, outcarl->set.str[STRING_SSL_ENGINE]))
      goto fail;
  }

#ifdef USE_ALTSVC
  if(data->asi) {
    outcarl->asi = Curl_altsvc_init();
    if(!outcarl->asi)
      goto fail;
    if(outcarl->set.str[STRING_ALTSVC])
      (void)Curl_altsvc_load(outcarl->asi, outcarl->set.str[STRING_ALTSVC]);
  }
#endif
#ifdef USE_HSTS
  if(data->hsts) {
    outcarl->hsts = Curl_hsts_init();
    if(!outcarl->hsts)
      goto fail;
    if(outcarl->set.str[STRING_HSTS])
      (void)Curl_hsts_loadfile(outcarl,
                               outcarl->hsts, outcarl->set.str[STRING_HSTS]);
    (void)Curl_hsts_loadcb(outcarl, outcarl->hsts);
  }
#endif
  /* Clone the resolver handle, if present, for the new handle */
  if(Curl_resolver_duphandle(outcarl,
                             &outcarl->state.async.resolver,
                             data->state.async.resolver))
    goto fail;

#ifdef USE_ARES
  {
    CARLcode rc;

    rc = Curl_set_dns_servers(outcarl, data->set.str[STRING_DNS_SERVERS]);
    if(rc && rc != CARLE_NOT_BUILT_IN)
      goto fail;

    rc = Curl_set_dns_interface(outcarl, data->set.str[STRING_DNS_INTERFACE]);
    if(rc && rc != CARLE_NOT_BUILT_IN)
      goto fail;

    rc = Curl_set_dns_local_ip4(outcarl, data->set.str[STRING_DNS_LOCAL_IP4]);
    if(rc && rc != CARLE_NOT_BUILT_IN)
      goto fail;

    rc = Curl_set_dns_local_ip6(outcarl, data->set.str[STRING_DNS_LOCAL_IP6]);
    if(rc && rc != CARLE_NOT_BUILT_IN)
      goto fail;
  }
#endif /* USE_ARES */

  Curl_convert_setup(outcarl);

  Curl_initinfo(outcarl);

  outcarl->magic = CARLEASY_MAGIC_NUMBER;

  /* we reach this point and thus we are OK */

  return outcarl;

  fail:

  if(outcarl) {
    carl_slist_free_all(outcarl->change.cookielist);
    outcarl->change.cookielist = NULL;
    Curl_safefree(outcarl->state.buffer);
    Curl_dyn_free(&outcarl->state.headerb);
    Curl_safefree(outcarl->change.url);
    Curl_safefree(outcarl->change.referer);
    Curl_altsvc_cleanup(&outcarl->asi);
    Curl_hsts_cleanup(&outcarl->hsts);
    Curl_freeset(outcarl);
    free(outcarl);
  }

  return NULL;
}

/*
 * carl_easy_reset() is an external interface that allows an app to re-
 * initialize a session handle to the default values.
 */
void carl_easy_reset(struct Curl_easy *data)
{
  Curl_free_request_state(data);

  /* zero out UserDefined data: */
  Curl_freeset(data);
  memset(&data->set, 0, sizeof(struct UserDefined));
  (void)Curl_init_userdefined(data);

  /* zero out Progress data: */
  memset(&data->progress, 0, sizeof(struct Progress));

  /* zero out PureInfo data: */
  Curl_initinfo(data);

  data->progress.flags |= PGRS_HIDE;
  data->state.current_speed = -1; /* init to negative == impossible */
  data->state.retrycount = 0;     /* reset the retry counter */

  /* zero out authentication data: */
  memset(&data->state.authhost, 0, sizeof(struct auth));
  memset(&data->state.authproxy, 0, sizeof(struct auth));

#if !defined(CARL_DISABLE_HTTP) && !defined(CARL_DISABLE_CRYPTO_AUTH)
  Curl_http_auth_cleanup_digest(data);
#endif
}

/*
 * carl_easy_pause() allows an application to pause or unpause a specific
 * transfer and direction. This function sets the full new state for the
 * current connection this easy handle operates on.
 *
 * NOTE: if you have the receiving paused and you call this function to remove
 * the pausing, you may get your write callback called at this point.
 *
 * Action is a bitmask consisting of CARLPAUSE_* bits in carl/carl.h
 *
 * NOTE: This is one of few API functions that are allowed to be called from
 * within a callback.
 */
CARLcode carl_easy_pause(struct Curl_easy *data, int action)
{
  struct SingleRequest *k;
  CARLcode result = CARLE_OK;
  int oldstate;
  int newstate;

  if(!GOOD_EASY_HANDLE(data) || !data->conn)
    /* crazy input, don't continue */
    return CARLE_BAD_FUNCTION_ARGUMENT;

  k = &data->req;
  oldstate = k->keepon & (KEEP_RECV_PAUSE| KEEP_SEND_PAUSE);

  /* first switch off both pause bits then set the new pause bits */
  newstate = (k->keepon &~ (KEEP_RECV_PAUSE| KEEP_SEND_PAUSE)) |
    ((action & CARLPAUSE_RECV)?KEEP_RECV_PAUSE:0) |
    ((action & CARLPAUSE_SEND)?KEEP_SEND_PAUSE:0);

  if((newstate & (KEEP_RECV_PAUSE| KEEP_SEND_PAUSE)) == oldstate) {
    /* Not changing any pause state, return */
    DEBUGF(infof(data, "pause: no change, early return\n"));
    return CARLE_OK;
  }

  /* Unpause parts in active mime tree. */
  if((k->keepon & ~newstate & KEEP_SEND_PAUSE) &&
     (data->mstate == CARLM_STATE_PERFORM ||
      data->mstate == CARLM_STATE_TOOFAST) &&
     data->state.fread_func == (carl_read_callback) Curl_mime_read) {
    Curl_mime_unpause(data->state.in);
  }

  /* put it back in the keepon */
  k->keepon = newstate;

  if(!(newstate & KEEP_RECV_PAUSE)) {
    Curl_http2_stream_pause(data, FALSE);

    if(data->state.tempcount) {
      /* there are buffers for sending that can be delivered as the receive
         pausing is lifted! */
      unsigned int i;
      unsigned int count = data->state.tempcount;
      struct tempbuf writebuf[3]; /* there can only be three */
      struct connectdata *conn = data->conn;
      struct Curl_easy *saved_data = NULL;

      /* copy the structs to allow for immediate re-pausing */
      for(i = 0; i < data->state.tempcount; i++) {
        writebuf[i] = data->state.tempwrite[i];
        Curl_dyn_init(&data->state.tempwrite[i].b, DYN_PAUSE_BUFFER);
      }
      data->state.tempcount = 0;

      /* set the connection's current owner */
      if(conn->data != data) {
        saved_data = conn->data;
        conn->data = data;
      }

      for(i = 0; i < count; i++) {
        /* even if one function returns error, this loops through and frees
           all buffers */
        if(!result)
          result = Curl_client_write(data, writebuf[i].type,
                                     Curl_dyn_ptr(&writebuf[i].b),
                                     Curl_dyn_len(&writebuf[i].b));
        Curl_dyn_free(&writebuf[i].b);
      }

      /* recover previous owner of the connection */
      if(saved_data)
        conn->data = saved_data;

      if(result)
        return result;
    }
  }

  /* if there's no error and we're not pausing both directions, we want
     to have this handle checked soon */
  if((newstate & (KEEP_RECV_PAUSE|KEEP_SEND_PAUSE)) !=
     (KEEP_RECV_PAUSE|KEEP_SEND_PAUSE)) {
    Curl_expire(data, 0, EXPIRE_RUN_NOW); /* get this handle going again */

    /* reset the too-slow time keeper */
    data->state.keeps_speed.tv_sec = 0;

    if(!data->state.tempcount)
      /* if not pausing again, force a recv/send check of this connection as
         the data might've been read off the socket already */
      data->conn->cselect_bits = CARL_CSELECT_IN | CARL_CSELECT_OUT;
    if(data->multi)
      Curl_update_timer(data->multi);
  }

  if(!data->state.done)
    /* This transfer may have been moved in or out of the bundle, update the
       corresponding socket callback, if used */
    Curl_updatesocket(data);

  return result;
}


static CARLcode easy_connection(struct Curl_easy *data,
                                carl_socket_t *sfd,
                                struct connectdata **connp)
{
  if(data == NULL)
    return CARLE_BAD_FUNCTION_ARGUMENT;

  /* only allow these to be called on handles with CARLOPT_CONNECT_ONLY */
  if(!data->set.connect_only) {
    failf(data, "CONNECT_ONLY is required!");
    return CARLE_UNSUPPORTED_PROTOCOL;
  }

  *sfd = Curl_getconnectinfo(data, connp);

  if(*sfd == CARL_SOCKET_BAD) {
    failf(data, "Failed to get recent socket");
    return CARLE_UNSUPPORTED_PROTOCOL;
  }

  return CARLE_OK;
}

/*
 * Receives data from the connected socket. Use after successful
 * carl_easy_perform() with CARLOPT_CONNECT_ONLY option.
 * Returns CARLE_OK on success, error code on error.
 */
CARLcode carl_easy_recv(struct Curl_easy *data, void *buffer, size_t buflen,
                        size_t *n)
{
  carl_socket_t sfd;
  CARLcode result;
  ssize_t n1;
  struct connectdata *c;

  if(Curl_is_in_callback(data))
    return CARLE_RECURSIVE_API_CALL;

  result = easy_connection(data, &sfd, &c);
  if(result)
    return result;

  if(!data->conn)
    /* on first invoke, the transfer has been detached from the connection and
       needs to be reattached */
    Curl_attach_connnection(data, c);

  *n = 0;
  result = Curl_read(data, sfd, buffer, buflen, &n1);

  if(result)
    return result;

  *n = (size_t)n1;

  return CARLE_OK;
}

/*
 * Sends data over the connected socket. Use after successful
 * carl_easy_perform() with CARLOPT_CONNECT_ONLY option.
 */
CARLcode carl_easy_send(struct Curl_easy *data, const void *buffer,
                        size_t buflen, size_t *n)
{
  carl_socket_t sfd;
  CARLcode result;
  ssize_t n1;
  struct connectdata *c = NULL;

  if(Curl_is_in_callback(data))
    return CARLE_RECURSIVE_API_CALL;

  result = easy_connection(data, &sfd, &c);
  if(result)
    return result;

  if(!data->conn)
    /* on first invoke, the transfer has been detached from the connection and
       needs to be reattached */
    Curl_attach_connnection(data, c);

  *n = 0;
  result = Curl_write(data, sfd, buffer, buflen, &n1);

  if(n1 == -1)
    return CARLE_SEND_ERROR;

  /* detect EAGAIN */
  if(!result && !n1)
    return CARLE_AGAIN;

  *n = (size_t)n1;

  return result;
}

/*
 * Wrapper to call functions in Curl_conncache_foreach()
 *
 * Returns always 0.
 */
static int conn_upkeep(struct Curl_easy *data,
                       struct connectdata *conn,
                       void *param)
{
  /* Param is unused. */
  (void)param;

  if(conn->handler->connection_check)
    /* Do a protocol-specific keepalive check on the connection. */
    conn->handler->connection_check(data, conn, CONNCHECK_KEEPALIVE);

  return 0; /* continue iteration */
}

static CARLcode upkeep(struct conncache *conn_cache, void *data)
{
  /* Loop over every connection and make connection alive. */
  Curl_conncache_foreach(data,
                         conn_cache,
                         data,
                         conn_upkeep);
  return CARLE_OK;
}

/*
 * Performs connection upkeep for the given session handle.
 */
CARLcode carl_easy_upkeep(struct Curl_easy *data)
{
  /* Verify that we got an easy handle we can work with. */
  if(!GOOD_EASY_HANDLE(data))
    return CARLE_BAD_FUNCTION_ARGUMENT;

  if(data->multi_easy) {
    /* Use the common function to keep connections alive. */
    return upkeep(&data->multi_easy->conn_cache, data);
  }
  else {
    /* No connections, so just return success */
    return CARLE_OK;
  }
}
