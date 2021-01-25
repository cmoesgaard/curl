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
#include <carl/carl.h>
#include "carl_range.h"
#include "sendf.h"
#include "strtoofft.h"

/* Only include this function if one or more of FTP, FILE are enabled. */
#if !defined(CARL_DISABLE_FTP) || !defined(CARL_DISABLE_FILE)

 /*
  Check if this is a range download, and if so, set the internal variables
  properly.
 */
CARLcode Curl_range(struct Curl_easy *data)
{
  carl_off_t from, to;
  char *ptr;
  char *ptr2;

  if(data->state.use_range && data->state.range) {
    CARLofft from_t;
    CARLofft to_t;
    from_t = carlx_strtoofft(data->state.range, &ptr, 0, &from);
    if(from_t == CARL_OFFT_FLOW)
      return CARLE_RANGE_ERROR;
    while(*ptr && (ISSPACE(*ptr) || (*ptr == '-')))
      ptr++;
    to_t = carlx_strtoofft(ptr, &ptr2, 0, &to);
    if(to_t == CARL_OFFT_FLOW)
      return CARLE_RANGE_ERROR;
    if((to_t == CARL_OFFT_INVAL) && !from_t) {
      /* X - */
      data->state.resume_from = from;
      DEBUGF(infof(data, "RANGE %" CARL_FORMAT_CARL_OFF_T " to end of file\n",
                   from));
    }
    else if((from_t == CARL_OFFT_INVAL) && !to_t) {
      /* -Y */
      data->req.maxdownload = to;
      data->state.resume_from = -to;
      DEBUGF(infof(data, "RANGE the last %" CARL_FORMAT_CARL_OFF_T " bytes\n",
                   to));
    }
    else {
      /* X-Y */
      carl_off_t totalsize;

      /* Ensure the range is sensible - to should follow from. */
      if(from > to)
        return CARLE_RANGE_ERROR;

      totalsize = to - from;
      if(totalsize == CARL_OFF_T_MAX)
        return CARLE_RANGE_ERROR;

      data->req.maxdownload = totalsize + 1; /* include last byte */
      data->state.resume_from = from;
      DEBUGF(infof(data, "RANGE from %" CARL_FORMAT_CARL_OFF_T
                   " getting %" CARL_FORMAT_CARL_OFF_T " bytes\n",
                   from, data->req.maxdownload));
    }
    DEBUGF(infof(data, "range-download from %" CARL_FORMAT_CARL_OFF_T
                 " to %" CARL_FORMAT_CARL_OFF_T ", totally %"
                 CARL_FORMAT_CARL_OFF_T " bytes\n",
                 from, to, data->req.maxdownload));
  }
  else
    data->req.maxdownload = -1;
  return CARLE_OK;
}

#endif
