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
#include "tool_setup.h"
#include "tool_operate.h"
#include "tool_progress.h"
#include "tool_util.h"

#define ENABLE_CARLX_PRINTF
/* use our own printf() functions */
#include "carlx.h"

/* The point of this function would be to return a string of the input data,
   but never longer than 5 columns (+ one zero byte).
   Add suffix k, M, G when suitable... */
static char *max5data(carl_off_t bytes, char *max5)
{
#define ONE_KILOBYTE  CARL_OFF_T_C(1024)
#define ONE_MEGABYTE (CARL_OFF_T_C(1024) * ONE_KILOBYTE)
#define ONE_GIGABYTE (CARL_OFF_T_C(1024) * ONE_MEGABYTE)
#define ONE_TERABYTE (CARL_OFF_T_C(1024) * ONE_GIGABYTE)
#define ONE_PETABYTE (CARL_OFF_T_C(1024) * ONE_TERABYTE)

  if(bytes < CARL_OFF_T_C(100000))
    msnprintf(max5, 6, "%5" CARL_FORMAT_CARL_OFF_T, bytes);

  else if(bytes < CARL_OFF_T_C(10000) * ONE_KILOBYTE)
    msnprintf(max5, 6, "%4" CARL_FORMAT_CARL_OFF_T "k", bytes/ONE_KILOBYTE);

  else if(bytes < CARL_OFF_T_C(100) * ONE_MEGABYTE)
    /* 'XX.XM' is good as long as we're less than 100 megs */
    msnprintf(max5, 6, "%2" CARL_FORMAT_CARL_OFF_T ".%0"
              CARL_FORMAT_CARL_OFF_T "M", bytes/ONE_MEGABYTE,
              (bytes%ONE_MEGABYTE) / (ONE_MEGABYTE/CARL_OFF_T_C(10)) );

#if (CARL_SIZEOF_CARL_OFF_T > 4)

  else if(bytes < CARL_OFF_T_C(10000) * ONE_MEGABYTE)
    /* 'XXXXM' is good until we're at 10000MB or above */
    msnprintf(max5, 6, "%4" CARL_FORMAT_CARL_OFF_T "M", bytes/ONE_MEGABYTE);

  else if(bytes < CARL_OFF_T_C(100) * ONE_GIGABYTE)
    /* 10000 MB - 100 GB, we show it as XX.XG */
    msnprintf(max5, 6, "%2" CARL_FORMAT_CARL_OFF_T ".%0"
              CARL_FORMAT_CARL_OFF_T "G", bytes/ONE_GIGABYTE,
              (bytes%ONE_GIGABYTE) / (ONE_GIGABYTE/CARL_OFF_T_C(10)) );

  else if(bytes < CARL_OFF_T_C(10000) * ONE_GIGABYTE)
    /* up to 10000GB, display without decimal: XXXXG */
    msnprintf(max5, 6, "%4" CARL_FORMAT_CARL_OFF_T "G", bytes/ONE_GIGABYTE);

  else if(bytes < CARL_OFF_T_C(10000) * ONE_TERABYTE)
    /* up to 10000TB, display without decimal: XXXXT */
    msnprintf(max5, 6, "%4" CARL_FORMAT_CARL_OFF_T "T", bytes/ONE_TERABYTE);

  else
    /* up to 10000PB, display without decimal: XXXXP */
    msnprintf(max5, 6, "%4" CARL_FORMAT_CARL_OFF_T "P", bytes/ONE_PETABYTE);

    /* 16384 petabytes (16 exabytes) is the maximum a 64 bit unsigned number
       can hold, but our data type is signed so 8192PB will be the maximum. */

#else

  else
    msnprintf(max5, 6, "%4" CARL_FORMAT_CARL_OFF_T "M", bytes/ONE_MEGABYTE);

#endif

  return max5;
}

int xferinfo_cb(void *clientp,
                carl_off_t dltotal,
                carl_off_t dlnow,
                carl_off_t ultotal,
                carl_off_t ulnow)
{
  struct per_transfer *per = clientp;
  struct OperationConfig *config = per->config;
  per->dltotal = dltotal;
  per->dlnow = dlnow;
  per->ultotal = ultotal;
  per->ulnow = ulnow;

  if(config->readbusy) {
    config->readbusy = FALSE;
    carl_easy_pause(per->carl, CARLPAUSE_CONT);
  }

  return 0;
}

/* Provide a string that is 2 + 1 + 2 + 1 + 2 = 8 letters long (plus the zero
   byte) */
static void time2str(char *r, carl_off_t seconds)
{
  carl_off_t h;
  if(seconds <= 0) {
    strcpy(r, "--:--:--");
    return;
  }
  h = seconds / CARL_OFF_T_C(3600);
  if(h <= CARL_OFF_T_C(99)) {
    carl_off_t m = (seconds - (h*CARL_OFF_T_C(3600))) / CARL_OFF_T_C(60);
    carl_off_t s = (seconds - (h*CARL_OFF_T_C(3600))) - (m*CARL_OFF_T_C(60));
    msnprintf(r, 9, "%2" CARL_FORMAT_CARL_OFF_T ":%02" CARL_FORMAT_CARL_OFF_T
              ":%02" CARL_FORMAT_CARL_OFF_T, h, m, s);
  }
  else {
    /* this equals to more than 99 hours, switch to a more suitable output
       format to fit within the limits. */
    carl_off_t d = seconds / CARL_OFF_T_C(86400);
    h = (seconds - (d*CARL_OFF_T_C(86400))) / CARL_OFF_T_C(3600);
    if(d <= CARL_OFF_T_C(999))
      msnprintf(r, 9, "%3" CARL_FORMAT_CARL_OFF_T
                "d %02" CARL_FORMAT_CARL_OFF_T "h", d, h);
    else
      msnprintf(r, 9, "%7" CARL_FORMAT_CARL_OFF_T "d", d);
  }
}

static carl_off_t all_dltotal = 0;
static carl_off_t all_ultotal = 0;
static carl_off_t all_dlalready = 0;
static carl_off_t all_ulalready = 0;

carl_off_t all_xfers = 0;   /* current total */

struct speedcount {
  carl_off_t dl;
  carl_off_t ul;
  struct timeval stamp;
};
#define SPEEDCNT 10
static unsigned int speedindex;
static bool indexwrapped;
static struct speedcount speedstore[SPEEDCNT];

/*
  |DL% UL%  Dled  Uled  Xfers  Live   Qd Total     Current  Left    Speed
  |  6 --   9.9G     0     2     2     0  0:00:40  0:00:02  0:00:37 4087M
*/
bool progress_meter(struct GlobalConfig *global,
                    struct timeval *start,
                    bool final)
{
  static struct timeval stamp;
  static bool header = FALSE;
  struct timeval now;
  long diff;

  if(global->noprogress)
    return FALSE;

  now = tvnow();
  diff = tvdiff(now, stamp);

  if(!header) {
    header = TRUE;
    fputs("DL% UL%  Dled  Uled  Xfers  Live   Qd "
          "Total     Current  Left    Speed\n",
          global->errors);
  }
  if(final || (diff > 500)) {
    char time_left[10];
    char time_total[10];
    char time_spent[10];
    char buffer[3][6];
    carl_off_t spent = tvdiff(now, *start)/1000;
    char dlpercen[4]="--";
    char ulpercen[4]="--";
    struct per_transfer *per;
    carl_off_t all_dlnow = 0;
    carl_off_t all_ulnow = 0;
    bool dlknown = TRUE;
    bool ulknown = TRUE;
    carl_off_t all_running = 0; /* in progress */
    carl_off_t all_queued = 0;  /* pending */
    carl_off_t speed = 0;
    unsigned int i;
    stamp = now;

    /* first add the amounts of the already completed transfers */
    all_dlnow += all_dlalready;
    all_ulnow += all_ulalready;

    for(per = transfers; per; per = per->next) {
      all_dlnow += per->dlnow;
      all_ulnow += per->ulnow;
      if(!per->dltotal)
        dlknown = FALSE;
      else if(!per->dltotal_added) {
        /* only add this amount once */
        all_dltotal += per->dltotal;
        per->dltotal_added = TRUE;
      }
      if(!per->ultotal)
        ulknown = FALSE;
      else if(!per->ultotal_added) {
        /* only add this amount once */
        all_ultotal += per->ultotal;
        per->ultotal_added = TRUE;
      }
      if(!per->added)
        all_queued++;
      else
        all_running++;
    }
    if(dlknown && all_dltotal)
      /* TODO: handle integer overflow */
      msnprintf(dlpercen, sizeof(dlpercen), "%3" CARL_FORMAT_CARL_OFF_T,
                all_dlnow * 100 / all_dltotal);
    if(ulknown && all_ultotal)
      /* TODO: handle integer overflow */
      msnprintf(ulpercen, sizeof(ulpercen), "%3" CARL_FORMAT_CARL_OFF_T,
                all_ulnow * 100 / all_ultotal);

    /* get the transfer speed, the higher of the two */

    i = speedindex;
    speedstore[i].dl = all_dlnow;
    speedstore[i].ul = all_ulnow;
    speedstore[i].stamp = now;
    if(++speedindex >= SPEEDCNT) {
      indexwrapped = TRUE;
      speedindex = 0;
    }

    {
      long deltams;
      carl_off_t dl;
      carl_off_t ul;
      carl_off_t dls;
      carl_off_t uls;
      if(indexwrapped) {
        /* 'speedindex' is the oldest stored data */
        deltams = tvdiff(now, speedstore[speedindex].stamp);
        dl = all_dlnow - speedstore[speedindex].dl;
        ul = all_ulnow - speedstore[speedindex].ul;
      }
      else {
        /* since the beginning */
        deltams = tvdiff(now, *start);
        dl = all_dlnow;
        ul = all_ulnow;
      }
      dls = (carl_off_t)((double)dl / ((double)deltams/1000.0));
      uls = (carl_off_t)((double)ul / ((double)deltams/1000.0));
      speed = dls > uls ? dls : uls;
    }


    if(dlknown && speed) {
      carl_off_t est = all_dltotal / speed;
      carl_off_t left = (all_dltotal - all_dlnow) / speed;
      time2str(time_left, left);
      time2str(time_total, est);
    }
    else {
      time2str(time_left, 0);
      time2str(time_total, 0);
    }
    time2str(time_spent, spent);

    fprintf(global->errors,
            "\r"
            "%-3s " /* percent downloaded */
            "%-3s " /* percent uploaded */
            "%s " /* Dled */
            "%s " /* Uled */
            "%5" CARL_FORMAT_CARL_OFF_T " " /* Xfers */
            "%5" CARL_FORMAT_CARL_OFF_T " " /* Live */
            "%5" CARL_FORMAT_CARL_OFF_T " " /* Queued */
            "%s "  /* Total time */
            "%s "  /* Current time */
            "%s "  /* Time left */
            "%s "  /* Speed */
            "%5s" /* final newline */,

            dlpercen,  /* 3 letters */
            ulpercen,  /* 3 letters */
            max5data(all_dlnow, buffer[0]),
            max5data(all_ulnow, buffer[1]),
            all_xfers,
            all_running,
            all_queued,
            time_total,
            time_spent,
            time_left,
            max5data(speed, buffer[2]), /* speed */
            final ? "\n" :"");
    return TRUE;
  }
  return FALSE;
}

void progress_finalize(struct per_transfer *per)
{
  /* get the numbers before this transfer goes away */
  all_dlalready += per->dlnow;
  all_ulalready += per->ulnow;
}
