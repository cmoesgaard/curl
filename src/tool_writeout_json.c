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

#define ENABLE_CARLX_PRINTF

/* use our own printf() functions */
#include "carlx.h"
#include "tool_cfgable.h"
#include "tool_writeout_json.h"
#include "tool_writeout.h"


static const char *http_version[] = {
  "0",   /* CARL_HTTP_VERSION_NONE */
  "1",   /* CARL_HTTP_VERSION_1_0 */
  "1.1", /* CARL_HTTP_VERSION_1_1 */
  "2",   /* CARL_HTTP_VERSION_2 */
  "3"    /* CARL_HTTP_VERSION_3 */
};

static void jsonEscape(FILE *stream, const char *in)
{
  const char *i = in;
  const char *in_end = in + strlen(in);

  for(; i < in_end; i++) {
    switch(*i) {
    case '\\':
      fputs("\\\\", stream);
      break;
    case '\"':
      fputs("\\\"", stream);
      break;
    case '\b':
      fputs("\\b", stream);
      break;
    case '\f':
      fputs("\\f", stream);
      break;
    case '\n':
      fputs("\\n", stream);
      break;
    case '\r':
      fputs("\\r", stream);
      break;
    case '\t':
      fputs("\\t", stream);
      break;
    default:
      if (*i < 32) {
        fprintf(stream, "u%04x", *i);
      }
      else {
        fputc(*i, stream);
      }
      break;
    }
  }
}

static int writeTime(FILE *str, CARL *carl, const char *key, CARLINFO ci)
{
  carl_off_t val = 0;
  if(CARLE_OK == carl_easy_getinfo(carl, ci, &val)) {
    carl_off_t s = val / 1000000l;
    carl_off_t ms = val % 1000000l;
    fprintf(str, "\"%s\":%" CARL_FORMAT_CARL_OFF_T
            ".%06" CARL_FORMAT_CARL_OFF_T, key, s, ms);
    return 1;
  }
  return 0;
}

static int writeString(FILE *str, CARL *carl, const char *key, CARLINFO ci)
{
  char *valp = NULL;
  if((CARLE_OK == carl_easy_getinfo(carl, ci, &valp)) && valp) {
    fprintf(str, "\"%s\":\"", key);
    jsonEscape(str, valp);
    fprintf(str, "\"");
    return 1;
  }
  return 0;
}

static int writeLong(FILE *str, CARL *carl, const char *key, CARLINFO ci,
                     struct per_transfer *per, const struct writeoutvar *wovar)
{
  if(wovar->id == VAR_NUM_HEADERS) {
    fprintf(str, "\"%s\":%ld", key, per->num_headers);
    return 1;
  }
  else {
    long val = 0;
    if(CARLE_OK == carl_easy_getinfo(carl, ci, &val)) {
      fprintf(str, "\"%s\":%ld", key, val);
      return 1;
    }
  }
  return 0;
}

static int writeOffset(FILE *str, CARL *carl, const char *key, CARLINFO ci)
{
  carl_off_t val = 0;
  if(CARLE_OK == carl_easy_getinfo(carl, ci, &val)) {
    fprintf(str, "\"%s\":%" CARL_FORMAT_CARL_OFF_T, key, val);
    return 1;
  }
  return 0;
}

static int writeFilename(FILE *str, const char *key, const char *filename)
{
  if(filename) {
    fprintf(str, "\"%s\":\"", key);
    jsonEscape(str, filename);
    fprintf(str, "\"");
  }
  else {
    fprintf(str, "\"%s\":null", key);
  }
  return 1;
}

static int writeVersion(FILE *str, CARL *carl, const char *key, CARLINFO ci)
{
  long version = 0;
  if(CARLE_OK == carl_easy_getinfo(carl, ci, &version) &&
     (version >= 0) &&
     (version < (long)(sizeof(http_version)/sizeof(char *)))) {
    fprintf(str, "\"%s\":\"%s\"", key, http_version[version]);
    return 1;
  }
  return 0;
}

void ourWriteOutJSON(const struct writeoutvar mappings[], CARL *carl,
                     struct per_transfer *per, FILE *stream)
{
  int i;

  fputs("{", stream);
  for(i = 0; mappings[i].name != NULL; i++) {
    const struct writeoutvar *wovar = &mappings[i];
    const char *name = mappings[i].name;
    CARLINFO cinfo = mappings[i].cinfo;
    int ok = 0;

    if(mappings[i].is_ctrl == 1) {
      continue;
    }

    switch(mappings[i].jsontype) {
    case JSON_STRING:
      ok = writeString(stream, carl, name, cinfo);
      break;
    case JSON_LONG:
      ok = writeLong(stream, carl, name, cinfo, per, wovar);
      break;
    case JSON_OFFSET:
      ok = writeOffset(stream, carl, name, cinfo);
      break;
    case JSON_TIME:
      ok = writeTime(stream, carl, name, cinfo);
      break;
    case JSON_FILENAME:
      ok = writeFilename(stream, name, per->outs.filename);
      break;
    case JSON_VERSION:
      ok = writeVersion(stream, carl, name, cinfo);
      break;
    default:
      break;
    }

    if(ok) {
      fputs(",", stream);
    }
  }

  fprintf(stream, "\"carl_version\":\"%s\"}", carl_version());
}
