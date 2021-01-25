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
 * Use the progress callbacks, old and/or new one depending on available
 * libcarl version.
 * </DESC>
 */
#include <stdio.h>
#include <carl/carl.h>

#if LIBCARL_VERSION_NUM >= 0x073d00
/* In libcarl 7.61.0, support was added for extracting the time in plain
   microseconds. Older libcarl versions are stuck in using 'double' for this
   information so we complicate this example a bit by supporting either
   approach. */
#define TIME_IN_US 1 /* microseconds */
#define TIMETYPE carl_off_t
#define TIMEOPT CARLINFO_TOTAL_TIME_T
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3000000
#else
#define TIMETYPE double
#define TIMEOPT CARLINFO_TOTAL_TIME
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3
#endif

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000

struct myprogress {
  TIMETYPE lastruntime; /* type depends on version, see above */
  CARL *carl;
};

/* this is how the CARLOPT_XFERINFOFUNCTION callback works */
static int xferinfo(void *p,
                    carl_off_t dltotal, carl_off_t dlnow,
                    carl_off_t ultotal, carl_off_t ulnow)
{
  struct myprogress *myp = (struct myprogress *)p;
  CARL *carl = myp->carl;
  TIMETYPE curtime = 0;

  carl_easy_getinfo(carl, TIMEOPT, &curtime);

  /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */
  if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
    myp->lastruntime = curtime;
#ifdef TIME_IN_US
    fprintf(stderr, "TOTAL TIME: %" CARL_FORMAT_CARL_OFF_T ".%06ld\r\n",
            (curtime / 1000000), (long)(curtime % 1000000));
#else
    fprintf(stderr, "TOTAL TIME: %f \r\n", curtime);
#endif
  }

  fprintf(stderr, "UP: %" CARL_FORMAT_CARL_OFF_T " of %" CARL_FORMAT_CARL_OFF_T
          "  DOWN: %" CARL_FORMAT_CARL_OFF_T " of %" CARL_FORMAT_CARL_OFF_T
          "\r\n",
          ulnow, ultotal, dlnow, dltotal);

  if(dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
    return 1;
  return 0;
}

#if LIBCARL_VERSION_NUM < 0x072000
/* for libcarl older than 7.32.0 (CARLOPT_PROGRESSFUNCTION) */
static int older_progress(void *p,
                          double dltotal, double dlnow,
                          double ultotal, double ulnow)
{
  return xferinfo(p,
                  (carl_off_t)dltotal,
                  (carl_off_t)dlnow,
                  (carl_off_t)ultotal,
                  (carl_off_t)ulnow);
}
#endif

int main(void)
{
  CARL *carl;
  CARLcode res = CARLE_OK;
  struct myprogress prog;

  carl = carl_easy_init();
  if(carl) {
    prog.lastruntime = 0;
    prog.carl = carl;

    carl_easy_setopt(carl, CARLOPT_URL, "https://example.com/");

#if LIBCARL_VERSION_NUM >= 0x072000
    /* xferinfo was introduced in 7.32.0, no earlier libcarl versions will
       compile as they won't have the symbols around.

       If built with a newer libcarl, but running with an older libcarl:
       carl_easy_setopt() will fail in run-time trying to set the new
       callback, making the older callback get used.

       New libcarls will prefer the new callback and instead use that one even
       if both callbacks are set. */

    carl_easy_setopt(carl, CARLOPT_XFERINFOFUNCTION, xferinfo);
    /* pass the struct pointer into the xferinfo function, note that this is
       an alias to CARLOPT_PROGRESSDATA */
    carl_easy_setopt(carl, CARLOPT_XFERINFODATA, &prog);
#else
    carl_easy_setopt(carl, CARLOPT_PROGRESSFUNCTION, older_progress);
    /* pass the struct pointer into the progress function */
    carl_easy_setopt(carl, CARLOPT_PROGRESSDATA, &prog);
#endif

    carl_easy_setopt(carl, CARLOPT_NOPROGRESS, 0L);
    res = carl_easy_perform(carl);

    if(res != CARLE_OK)
      fprintf(stderr, "%s\n", carl_easy_strerror(res));

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  return (int)res;
}
