#ifndef CARLINC_MULTI_H
#define CARLINC_MULTI_H
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
/*
  This is an "external" header file. Don't give away any internals here!

  GOALS

  o Enable a "pull" interface. The application that uses libcarl decides where
    and when to ask libcarl to get/send data.

  o Enable multiple simultaneous transfers in the same thread without making it
    complicated for the application.

  o Enable the application to select() on its own file descriptors and carl's
    file descriptors simultaneous easily.

*/

/*
 * This header file should not really need to include "carl.h" since carl.h
 * itself includes this file and we expect user applications to do #include
 * <carl/carl.h> without the need for especially including multi.h.
 *
 * For some reason we added this include here at one point, and rather than to
 * break existing (wrongly written) libcarl applications, we leave it as-is
 * but with this warning attached.
 */
#include "carl.h"

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(BUILDING_LIBCARL) || defined(CARL_STRICTER)
typedef struct Curl_multi CARLM;
#else
typedef void CARLM;
#endif

typedef enum {
  CARLM_CALL_MULTI_PERFORM = -1, /* please call carl_multi_perform() or
                                    carl_multi_socket*() soon */
  CARLM_OK,
  CARLM_BAD_HANDLE,      /* the passed-in handle is not a valid CARLM handle */
  CARLM_BAD_EASY_HANDLE, /* an easy handle was not good/valid */
  CARLM_OUT_OF_MEMORY,   /* if you ever get this, you're in deep sh*t */
  CARLM_INTERNAL_ERROR,  /* this is a libcarl bug */
  CARLM_BAD_SOCKET,      /* the passed in socket argument did not match */
  CARLM_UNKNOWN_OPTION,  /* carl_multi_setopt() with unsupported option */
  CARLM_ADDED_ALREADY,   /* an easy handle already added to a multi handle was
                            attempted to get added - again */
  CARLM_RECURSIVE_API_CALL, /* an api function was called from inside a
                               callback */
  CARLM_WAKEUP_FAILURE,  /* wakeup is unavailable or failed */
  CARLM_BAD_FUNCTION_ARGUMENT,  /* function called with a bad parameter */
  CARLM_LAST
} CARLMcode;

/* just to make code nicer when using carl_multi_socket() you can now check
   for CARLM_CALL_MULTI_SOCKET too in the same style it works for
   carl_multi_perform() and CARLM_CALL_MULTI_PERFORM */
#define CARLM_CALL_MULTI_SOCKET CARLM_CALL_MULTI_PERFORM

/* bitmask bits for CARLMOPT_PIPELINING */
#define CARLPIPE_NOTHING   0L
#define CARLPIPE_HTTP1     1L
#define CARLPIPE_MULTIPLEX 2L

typedef enum {
  CARLMSG_NONE, /* first, not used */
  CARLMSG_DONE, /* This easy handle has completed. 'result' contains
                   the CARLcode of the transfer */
  CARLMSG_LAST /* last, not used */
} CARLMSG;

struct CARLMsg {
  CARLMSG msg;       /* what this message means */
  CARL *easy_handle; /* the handle it concerns */
  union {
    void *whatever;    /* message-specific data */
    CARLcode result;   /* return code for transfer */
  } data;
};
typedef struct CARLMsg CARLMsg;

/* Based on poll(2) structure and values.
 * We don't use pollfd and POLL* constants explicitly
 * to cover platforms without poll(). */
#define CARL_WAIT_POLLIN    0x0001
#define CARL_WAIT_POLLPRI   0x0002
#define CARL_WAIT_POLLOUT   0x0004

struct carl_waitfd {
  carl_socket_t fd;
  short events;
  short revents; /* not supported yet */
};

/*
 * Name:    carl_multi_init()
 *
 * Desc:    inititalize multi-style carl usage
 *
 * Returns: a new CARLM handle to use in all 'carl_multi' functions.
 */
CARL_EXTERN CARLM *carl_multi_init(void);

/*
 * Name:    carl_multi_add_handle()
 *
 * Desc:    add a standard carl handle to the multi stack
 *
 * Returns: CARLMcode type, general multi error code.
 */
CARL_EXTERN CARLMcode carl_multi_add_handle(CARLM *multi_handle,
                                            CARL *carl_handle);

 /*
  * Name:    carl_multi_remove_handle()
  *
  * Desc:    removes a carl handle from the multi stack again
  *
  * Returns: CARLMcode type, general multi error code.
  */
CARL_EXTERN CARLMcode carl_multi_remove_handle(CARLM *multi_handle,
                                               CARL *carl_handle);

 /*
  * Name:    carl_multi_fdset()
  *
  * Desc:    Ask carl for its fd_set sets. The app can use these to select() or
  *          poll() on. We want carl_multi_perform() called as soon as one of
  *          them are ready.
  *
  * Returns: CARLMcode type, general multi error code.
  */
CARL_EXTERN CARLMcode carl_multi_fdset(CARLM *multi_handle,
                                       fd_set *read_fd_set,
                                       fd_set *write_fd_set,
                                       fd_set *exc_fd_set,
                                       int *max_fd);

/*
 * Name:     carl_multi_wait()
 *
 * Desc:     Poll on all fds within a CARLM set as well as any
 *           additional fds passed to the function.
 *
 * Returns:  CARLMcode type, general multi error code.
 */
CARL_EXTERN CARLMcode carl_multi_wait(CARLM *multi_handle,
                                      struct carl_waitfd extra_fds[],
                                      unsigned int extra_nfds,
                                      int timeout_ms,
                                      int *ret);

/*
 * Name:     carl_multi_poll()
 *
 * Desc:     Poll on all fds within a CARLM set as well as any
 *           additional fds passed to the function.
 *
 * Returns:  CARLMcode type, general multi error code.
 */
CARL_EXTERN CARLMcode carl_multi_poll(CARLM *multi_handle,
                                      struct carl_waitfd extra_fds[],
                                      unsigned int extra_nfds,
                                      int timeout_ms,
                                      int *ret);

/*
 * Name:     carl_multi_wakeup()
 *
 * Desc:     wakes up a sleeping carl_multi_poll call.
 *
 * Returns:  CARLMcode type, general multi error code.
 */
CARL_EXTERN CARLMcode carl_multi_wakeup(CARLM *multi_handle);

 /*
  * Name:    carl_multi_perform()
  *
  * Desc:    When the app thinks there's data available for carl it calls this
  *          function to read/write whatever there is right now. This returns
  *          as soon as the reads and writes are done. This function does not
  *          require that there actually is data available for reading or that
  *          data can be written, it can be called just in case. It returns
  *          the number of handles that still transfer data in the second
  *          argument's integer-pointer.
  *
  * Returns: CARLMcode type, general multi error code. *NOTE* that this only
  *          returns errors etc regarding the whole multi stack. There might
  *          still have occurred problems on individual transfers even when
  *          this returns OK.
  */
CARL_EXTERN CARLMcode carl_multi_perform(CARLM *multi_handle,
                                         int *running_handles);

 /*
  * Name:    carl_multi_cleanup()
  *
  * Desc:    Cleans up and removes a whole multi stack. It does not free or
  *          touch any individual easy handles in any way. We need to define
  *          in what state those handles will be if this function is called
  *          in the middle of a transfer.
  *
  * Returns: CARLMcode type, general multi error code.
  */
CARL_EXTERN CARLMcode carl_multi_cleanup(CARLM *multi_handle);

/*
 * Name:    carl_multi_info_read()
 *
 * Desc:    Ask the multi handle if there's any messages/informationals from
 *          the individual transfers. Messages include informationals such as
 *          error code from the transfer or just the fact that a transfer is
 *          completed. More details on these should be written down as well.
 *
 *          Repeated calls to this function will return a new struct each
 *          time, until a special "end of msgs" struct is returned as a signal
 *          that there is no more to get at this point.
 *
 *          The data the returned pointer points to will not survive calling
 *          carl_multi_cleanup().
 *
 *          The 'CARLMsg' struct is meant to be very simple and only contain
 *          very basic information. If more involved information is wanted,
 *          we will provide the particular "transfer handle" in that struct
 *          and that should/could/would be used in subsequent
 *          carl_easy_getinfo() calls (or similar). The point being that we
 *          must never expose complex structs to applications, as then we'll
 *          undoubtably get backwards compatibility problems in the future.
 *
 * Returns: A pointer to a filled-in struct, or NULL if it failed or ran out
 *          of structs. It also writes the number of messages left in the
 *          queue (after this read) in the integer the second argument points
 *          to.
 */
CARL_EXTERN CARLMsg *carl_multi_info_read(CARLM *multi_handle,
                                          int *msgs_in_queue);

/*
 * Name:    carl_multi_strerror()
 *
 * Desc:    The carl_multi_strerror function may be used to turn a CARLMcode
 *          value into the equivalent human readable error string.  This is
 *          useful for printing meaningful error messages.
 *
 * Returns: A pointer to a null-terminated error message.
 */
CARL_EXTERN const char *carl_multi_strerror(CARLMcode);

/*
 * Name:    carl_multi_socket() and
 *          carl_multi_socket_all()
 *
 * Desc:    An alternative version of carl_multi_perform() that allows the
 *          application to pass in one of the file descriptors that have been
 *          detected to have "action" on them and let libcarl perform.
 *          See man page for details.
 */
#define CARL_POLL_NONE   0
#define CARL_POLL_IN     1
#define CARL_POLL_OUT    2
#define CARL_POLL_INOUT  3
#define CARL_POLL_REMOVE 4

#define CARL_SOCKET_TIMEOUT CARL_SOCKET_BAD

#define CARL_CSELECT_IN   0x01
#define CARL_CSELECT_OUT  0x02
#define CARL_CSELECT_ERR  0x04

typedef int (*carl_socket_callback)(CARL *easy,      /* easy handle */
                                    carl_socket_t s, /* socket */
                                    int what,        /* see above */
                                    void *userp,     /* private callback
                                                        pointer */
                                    void *socketp);  /* private socket
                                                        pointer */
/*
 * Name:    carl_multi_timer_callback
 *
 * Desc:    Called by libcarl whenever the library detects a change in the
 *          maximum number of milliseconds the app is allowed to wait before
 *          carl_multi_socket() or carl_multi_perform() must be called
 *          (to allow libcarl's timed events to take place).
 *
 * Returns: The callback should return zero.
 */
typedef int (*carl_multi_timer_callback)(CARLM *multi,    /* multi handle */
                                         long timeout_ms, /* see above */
                                         void *userp);    /* private callback
                                                             pointer */

CARL_EXTERN CARLMcode carl_multi_socket(CARLM *multi_handle, carl_socket_t s,
                                        int *running_handles);

CARL_EXTERN CARLMcode carl_multi_socket_action(CARLM *multi_handle,
                                               carl_socket_t s,
                                               int ev_bitmask,
                                               int *running_handles);

CARL_EXTERN CARLMcode carl_multi_socket_all(CARLM *multi_handle,
                                            int *running_handles);

#ifndef CARL_ALLOW_OLD_MULTI_SOCKET
/* This macro below was added in 7.16.3 to push users who recompile to use
   the new carl_multi_socket_action() instead of the old carl_multi_socket()
*/
#define carl_multi_socket(x,y,z) carl_multi_socket_action(x,y,0,z)
#endif

/*
 * Name:    carl_multi_timeout()
 *
 * Desc:    Returns the maximum number of milliseconds the app is allowed to
 *          wait before carl_multi_socket() or carl_multi_perform() must be
 *          called (to allow libcarl's timed events to take place).
 *
 * Returns: CARLM error code.
 */
CARL_EXTERN CARLMcode carl_multi_timeout(CARLM *multi_handle,
                                         long *milliseconds);

typedef enum {
  /* This is the socket callback function pointer */
  CARLOPT(CARLMOPT_SOCKETFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 1),

  /* This is the argument passed to the socket callback */
  CARLOPT(CARLMOPT_SOCKETDATA, CARLOPTTYPE_OBJECTPOINT, 2),

    /* set to 1 to enable pipelining for this multi handle */
  CARLOPT(CARLMOPT_PIPELINING, CARLOPTTYPE_LONG, 3),

   /* This is the timer callback function pointer */
  CARLOPT(CARLMOPT_TIMERFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 4),

  /* This is the argument passed to the timer callback */
  CARLOPT(CARLMOPT_TIMERDATA, CARLOPTTYPE_OBJECTPOINT, 5),

  /* maximum number of entries in the connection cache */
  CARLOPT(CARLMOPT_MAXCONNECTS, CARLOPTTYPE_LONG, 6),

  /* maximum number of (pipelining) connections to one host */
  CARLOPT(CARLMOPT_MAX_HOST_CONNECTIONS, CARLOPTTYPE_LONG, 7),

  /* maximum number of requests in a pipeline */
  CARLOPT(CARLMOPT_MAX_PIPELINE_LENGTH, CARLOPTTYPE_LONG, 8),

  /* a connection with a content-length longer than this
     will not be considered for pipelining */
  CARLOPT(CARLMOPT_CONTENT_LENGTH_PENALTY_SIZE, CARLOPTTYPE_OFF_T, 9),

  /* a connection with a chunk length longer than this
     will not be considered for pipelining */
  CARLOPT(CARLMOPT_CHUNK_LENGTH_PENALTY_SIZE, CARLOPTTYPE_OFF_T, 10),

  /* a list of site names(+port) that are blocked from pipelining */
  CARLOPT(CARLMOPT_PIPELINING_SITE_BL, CARLOPTTYPE_OBJECTPOINT, 11),

  /* a list of server types that are blocked from pipelining */
  CARLOPT(CARLMOPT_PIPELINING_SERVER_BL, CARLOPTTYPE_OBJECTPOINT, 12),

  /* maximum number of open connections in total */
  CARLOPT(CARLMOPT_MAX_TOTAL_CONNECTIONS, CARLOPTTYPE_LONG, 13),

   /* This is the server push callback function pointer */
  CARLOPT(CARLMOPT_PUSHFUNCTION, CARLOPTTYPE_FUNCTIONPOINT, 14),

  /* This is the argument passed to the server push callback */
  CARLOPT(CARLMOPT_PUSHDATA, CARLOPTTYPE_OBJECTPOINT, 15),

  /* maximum number of concurrent streams to support on a connection */
  CARLOPT(CARLMOPT_MAX_CONCURRENT_STREAMS, CARLOPTTYPE_LONG, 16),

  CARLMOPT_LASTENTRY /* the last unused */
} CARLMoption;


/*
 * Name:    carl_multi_setopt()
 *
 * Desc:    Sets options for the multi handle.
 *
 * Returns: CARLM error code.
 */
CARL_EXTERN CARLMcode carl_multi_setopt(CARLM *multi_handle,
                                        CARLMoption option, ...);


/*
 * Name:    carl_multi_assign()
 *
 * Desc:    This function sets an association in the multi handle between the
 *          given socket and a private pointer of the application. This is
 *          (only) useful for carl_multi_socket uses.
 *
 * Returns: CARLM error code.
 */
CARL_EXTERN CARLMcode carl_multi_assign(CARLM *multi_handle,
                                        carl_socket_t sockfd, void *sockp);


/*
 * Name: carl_push_callback
 *
 * Desc: This callback gets called when a new stream is being pushed by the
 *       server. It approves or denies the new stream. It can also decide
 *       to completely fail the connection.
 *
 * Returns: CARL_PUSH_OK, CARL_PUSH_DENY or CARL_PUSH_ERROROUT
 */
#define CARL_PUSH_OK       0
#define CARL_PUSH_DENY     1
#define CARL_PUSH_ERROROUT 2 /* added in 7.72.0 */

struct carl_pushheaders;  /* forward declaration only */

CARL_EXTERN char *carl_pushheader_bynum(struct carl_pushheaders *h,
                                        size_t num);
CARL_EXTERN char *carl_pushheader_byname(struct carl_pushheaders *h,
                                         const char *name);

typedef int (*carl_push_callback)(CARL *parent,
                                  CARL *easy,
                                  size_t num_headers,
                                  struct carl_pushheaders *headers,
                                  void *userp);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif
