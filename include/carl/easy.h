#ifndef CARLINC_EASY_H
#define CARLINC_EASY_H
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
#ifdef  __cplusplus
extern "C" {
#endif

/* Flag bits in the carl_blob struct: */
#define CARL_BLOB_COPY   1 /* tell libcarl to copy the data */
#define CARL_BLOB_NOCOPY 0 /* tell libcarl to NOT copy the data */

struct carl_blob {
  void *data;
  size_t len;
  unsigned int flags; /* bit 0 is defined, the rest are reserved and should be
                         left zeroes */
};

CARL_EXTERN CARL *carl_easy_init(void);
CARL_EXTERN CARLcode carl_easy_setopt(CARL *carl, CARLoption option, ...);
CARL_EXTERN CARLcode carl_easy_perform(CARL *carl);
CARL_EXTERN void carl_easy_cleanup(CARL *carl);

/*
 * NAME carl_easy_getinfo()
 *
 * DESCRIPTION
 *
 * Request internal information from the carl session with this function.  The
 * third argument MUST be a pointer to a long, a pointer to a char * or a
 * pointer to a double (as the documentation describes elsewhere).  The data
 * pointed to will be filled in accordingly and can be relied upon only if the
 * function returns CARLE_OK.  This function is intended to get used *AFTER* a
 * performed transfer, all results from this function are undefined until the
 * transfer is completed.
 */
CARL_EXTERN CARLcode carl_easy_getinfo(CARL *carl, CARLINFO info, ...);


/*
 * NAME carl_easy_duphandle()
 *
 * DESCRIPTION
 *
 * Creates a new carl session handle with the same options set for the handle
 * passed in. Duplicating a handle could only be a matter of cloning data and
 * options, internal state info and things like persistent connections cannot
 * be transferred. It is useful in multithreaded applications when you can run
 * carl_easy_duphandle() for each new thread to avoid a series of identical
 * carl_easy_setopt() invokes in every thread.
 */
CARL_EXTERN CARL *carl_easy_duphandle(CARL *carl);

/*
 * NAME carl_easy_reset()
 *
 * DESCRIPTION
 *
 * Re-initializes a CARL handle to the default values. This puts back the
 * handle to the same state as it was in when it was just created.
 *
 * It does keep: live connections, the Session ID cache, the DNS cache and the
 * cookies.
 */
CARL_EXTERN void carl_easy_reset(CARL *carl);

/*
 * NAME carl_easy_recv()
 *
 * DESCRIPTION
 *
 * Receives data from the connected socket. Use after successful
 * carl_easy_perform() with CARLOPT_CONNECT_ONLY option.
 */
CARL_EXTERN CARLcode carl_easy_recv(CARL *carl, void *buffer, size_t buflen,
                                    size_t *n);

/*
 * NAME carl_easy_send()
 *
 * DESCRIPTION
 *
 * Sends data over the connected socket. Use after successful
 * carl_easy_perform() with CARLOPT_CONNECT_ONLY option.
 */
CARL_EXTERN CARLcode carl_easy_send(CARL *carl, const void *buffer,
                                    size_t buflen, size_t *n);


/*
 * NAME carl_easy_upkeep()
 *
 * DESCRIPTION
 *
 * Performs connection upkeep for the given session handle.
 */
CARL_EXTERN CARLcode carl_easy_upkeep(CARL *carl);

#ifdef  __cplusplus
}
#endif

#endif
