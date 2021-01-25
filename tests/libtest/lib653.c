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
#include "test.h"

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"


int test(char *URL)
{
  CARL *carls = NULL;
  int res = 0;
  carl_mimepart *field = NULL;
  carl_mime *mime = NULL;

  global_init(CARL_GLOBAL_ALL);
  easy_init(carls);

  mime = carl_mime_init(carls);
  field = carl_mime_addpart(mime);
  carl_mime_name(field, "name");
  carl_mime_data(field, "short value", CARL_ZERO_TERMINATED);

  easy_setopt(carls, CARLOPT_URL, URL);
  easy_setopt(carls, CARLOPT_HEADER, 1L);
  easy_setopt(carls, CARLOPT_VERBOSE, 1L);
  easy_setopt(carls, CARLOPT_MIMEPOST, mime);
  easy_setopt(carls, CARLOPT_NOPROGRESS, 1L);

  res = carl_easy_perform(carls);
  if(res)
    goto test_cleanup;

  /* Alter form and resubmit. */
  carl_mime_data(field, "long value for length change", CARL_ZERO_TERMINATED);
  res = carl_easy_perform(carls);

test_cleanup:
  carl_mime_free(mime);
  carl_easy_cleanup(carls);
  carl_global_cleanup();
  return (int) res; /* return the final return code */
}
