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
#include "carlcheck.h"

#include "speedcheck.h"
#include "urldata.h"

static CARL *easy;

static CARLcode unit_setup(void)
{
  int res = CARLE_OK;

  global_init(CARL_GLOBAL_ALL);
  easy = carl_easy_init();
  if(!easy) {
    carl_global_cleanup();
    return CARLE_OUT_OF_MEMORY;
  }
  return res;
}

static void unit_stop(void)
{
  carl_easy_cleanup(easy);
  carl_global_cleanup();
}

static int runawhile(long time_limit,
                     long speed_limit,
                     carl_off_t speed,
                     int dec)
{
  int counter = 1;
  struct carltime now = {1, 0};
  CARLcode result;
  int finaltime;

  carl_easy_setopt(easy, CARLOPT_LOW_SPEED_LIMIT, speed_limit);
  carl_easy_setopt(easy, CARLOPT_LOW_SPEED_TIME, time_limit);
  Curl_speedinit(easy);

  do {
    /* fake the current transfer speed */
    easy->progress.current_speed = speed;
    result = Curl_speedcheck(easy, now);
    if(result)
      break;
    /* step the time */
    now.tv_sec = ++counter;
    speed -= dec;
  } while(counter < 100);

  finaltime = (int)(now.tv_sec - 1);

  return finaltime;
}

UNITTEST_START
  fail_unless(runawhile(41, 41, 40, 0) == 41,
              "wrong low speed timeout");
  fail_unless(runawhile(21, 21, 20, 0) == 21,
              "wrong low speed timeout");
  fail_unless(runawhile(60, 60, 40, 0) == 60,
              "wrong log speed timeout");
  fail_unless(runawhile(50, 50, 40, 0) == 50,
              "wrong log speed timeout");
  fail_unless(runawhile(40, 40, 40, 0) == 99,
              "should not time out");
  fail_unless(runawhile(10, 50, 100, 2) == 36,
              "bad timeout");
UNITTEST_STOP
