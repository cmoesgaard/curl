/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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
 * The Strict-Transport-Security header is defined in RFC 6797:
 * https://tools.ietf.org/html/rfc6797
 */
#include "carl_setup.h"

#if !defined(CARL_DISABLE_HTTP) && defined(USE_HSTS)
#include <carl/carl.h>
#include "urldata.h"
#include "llist.h"
#include "hsts.h"
#include "carl_get_line.h"
#include "strcase.h"
#include "sendf.h"
#include "strtoofft.h"
#include "parsedate.h"
#include "rand.h"
#include "rename.h"

/* The last 3 #include files should be in this order */
#include "carl_printf.h"
#include "carl_memory.h"
#include "memdebug.h"

#define MAX_HSTS_LINE 4095
#define MAX_HSTS_HOSTLEN 256
#define MAX_HSTS_HOSTLENSTR "256"
#define MAX_HSTS_SUBLEN 4
#define MAX_HSTS_SUBLENSTR "4"
#define MAX_HSTS_DATELEN 64
#define MAX_HSTS_DATELENSTR "64"

#ifdef DEBUGBUILD
/* to play well with debug builds, we can *set* a fixed time this will
   return */
time_t deltatime; /* allow for "adjustments" for unit test purposes */
static time_t debugtime(void *unused)
{
  char *timestr = getenv("CARL_TIME");
  (void)unused;
  if(timestr) {
    unsigned long val = strtol(timestr, NULL, 10) + deltatime;
    return (time_t)val;
  }
  return time(NULL);
}
#define time(x) debugtime(x)
#endif

struct hsts *Curl_hsts_init(void)
{
  struct hsts *h = calloc(sizeof(struct hsts), 1);
  if(h) {
    Curl_llist_init(&h->list, NULL);
  }
  return h;
}

static void hsts_free(struct stsentry *e)
{
  free((char *)e->host);
  free(e);
}

void Curl_hsts_cleanup(struct hsts **hp)
{
  struct hsts *h = *hp;
  if(h) {
    struct Curl_llist_element *e;
    struct Curl_llist_element *n;
    for(e = h->list.head; e; e = n) {
      struct stsentry *sts = e->ptr;
      n = e->next;
      hsts_free(sts);
    }
    free(h->filename);
    free(h);
    *hp = NULL;
  }
}

static struct stsentry *hsts_entry(void)
{
  return calloc(sizeof(struct stsentry), 1);
}

static CARLcode hsts_create(struct hsts *h,
                            const char *hostname,
                            bool subdomains,
                            carl_off_t expires)
{
  struct stsentry *sts = hsts_entry();
  if(!sts)
    return CARLE_OUT_OF_MEMORY;

  sts->expires = expires;
  sts->includeSubDomains = subdomains;
  sts->host = strdup(hostname);
  if(!sts->host) {
    free(sts);
    return CARLE_OUT_OF_MEMORY;
  }
  Curl_llist_insert_next(&h->list, h->list.tail, sts, &sts->node);
  return CARLE_OK;
}

CARLcode Curl_hsts_parse(struct hsts *h, const char *hostname,
                         const char *header)
{
  const char *p = header;
  carl_off_t expires = 0;
  bool gotma = FALSE;
  bool gotinc = FALSE;
  bool subdomains = FALSE;
  struct stsentry *sts;
  time_t now = time(NULL);

  do {
    while(*p && ISSPACE(*p))
      p++;
    if(Curl_strncasecompare("max-age=", p, 8)) {
      bool quoted = FALSE;
      CARLofft offt;
      char *endp;

      if(gotma)
        return CARLE_BAD_FUNCTION_ARGUMENT;

      p += 8;
      while(*p && ISSPACE(*p))
        p++;
      if(*p == '\"') {
        p++;
        quoted = TRUE;
      }
      offt = carlx_strtoofft(p, &endp, 10, &expires);
      if(offt == CARL_OFFT_FLOW)
        expires = CARL_OFF_T_MAX;
      else if(offt)
        /* invalid max-age */
        return CARLE_BAD_FUNCTION_ARGUMENT;
      p = endp;
      if(quoted) {
        if(*p != '\"')
          return CARLE_BAD_FUNCTION_ARGUMENT;
        p++;
      }
      gotma = TRUE;
    }
    else if(Curl_strncasecompare("includesubdomains", p, 17)) {
      if(gotinc)
        return CARLE_BAD_FUNCTION_ARGUMENT;
      subdomains = TRUE;
      p += 17;
      gotinc = TRUE;
    }
    else {
      /* unknown directive, do a lame attempt to skip */
      while(*p && (*p != ';'))
        p++;
    }

    while(*p && ISSPACE(*p))
      p++;
    if(*p == ';')
      p++;
  } while (*p);

  if(!gotma)
    /* max-age is mandatory */
    return CARLE_BAD_FUNCTION_ARGUMENT;

  if(!expires) {
    /* remove the entry if present verbatim (without subdomain match) */
    sts = Curl_hsts(h, hostname, FALSE);
    if(sts) {
      Curl_llist_remove(&h->list, &sts->node, NULL);
      hsts_free(sts);
    }
    return CARLE_OK;
  }

  if(CARL_OFF_T_MAX - now < expires)
    /* would overflow, use maximum value */
    expires = CARL_OFF_T_MAX;
  else
    expires += now;

  /* check if it already exists */
  sts = Curl_hsts(h, hostname, FALSE);
  if(sts) {
    /* just update these fields */
    sts->expires = expires;
    sts->includeSubDomains = subdomains;
  }
  else
    return hsts_create(h, hostname, subdomains, expires);

  return CARLE_OK;
}

/*
 * Return TRUE if the given host name is currently an HSTS one.
 *
 * The 'subdomain' argument tells the function if subdomain matching should be
 * attempted.
 */
struct stsentry *Curl_hsts(struct hsts *h, const char *hostname,
                           bool subdomain)
{
  if(h) {
    time_t now = time(NULL);
    size_t hlen = strlen(hostname);
    struct Curl_llist_element *e;
    struct Curl_llist_element *n;
    for(e = h->list.head; e; e = n) {
      struct stsentry *sts = e->ptr;
      n = e->next;
      if(sts->expires <= now) {
        /* remove expired entries */
        Curl_llist_remove(&h->list, &sts->node, NULL);
        hsts_free(sts);
        continue;
      }
      if(subdomain && sts->includeSubDomains) {
        size_t ntail = strlen(sts->host);
        if(ntail < hlen) {
          size_t offs = hlen - ntail;
          if((hostname[offs-1] == '.') &&
             Curl_strncasecompare(&hostname[offs], sts->host, ntail))
            return sts;
        }
      }
      if(Curl_strcasecompare(hostname, sts->host))
        return sts;
    }
  }
  return NULL; /* no match */
}

/*
 * Send this HSTS entry to the write callback.
 */
static CARLcode hsts_push(struct Curl_easy *data,
                          struct carl_index *i,
                          struct stsentry *sts,
                          bool *stop)
{
  struct carl_hstsentry e;
  CARLSTScode sc;
  struct tm stamp;
  CARLcode result;

  e.name = (char *)sts->host;
  e.namelen = strlen(sts->host);
  e.includeSubDomains = sts->includeSubDomains;

  result = Curl_gmtime(sts->expires, &stamp);
  if(result)
    return result;

  msnprintf(e.expire, sizeof(e.expire), "%d%02d%02d %02d:%02d:%02d",
            stamp.tm_year + 1900, stamp.tm_mon + 1, stamp.tm_mday,
            stamp.tm_hour, stamp.tm_min, stamp.tm_sec);

  sc = data->set.hsts_write(data, &e, i,
                            data->set.hsts_write_userp);
  *stop = (sc != CARLSTS_OK);
  return sc == CARLSTS_FAIL ? CARLE_BAD_FUNCTION_ARGUMENT : CARLE_OK;
}

/*
 * Write this single hsts entry to a single output line
 */
static CARLcode hsts_out(struct stsentry *sts, FILE *fp)
{
  struct tm stamp;
  CARLcode result = Curl_gmtime(sts->expires, &stamp);
  if(result)
    return result;

  fprintf(fp, "%s%s \"%d%02d%02d %02d:%02d:%02d\"\n",
          sts->includeSubDomains ? ".": "", sts->host,
          stamp.tm_year + 1900, stamp.tm_mon + 1, stamp.tm_mday,
          stamp.tm_hour, stamp.tm_min, stamp.tm_sec);
  return CARLE_OK;
}


/*
 * Curl_https_save() writes the HSTS cache to file and callback.
 */
CARLcode Curl_hsts_save(struct Curl_easy *data, struct hsts *h,
                        const char *file)
{
  struct Curl_llist_element *e;
  struct Curl_llist_element *n;
  CARLcode result = CARLE_OK;
  FILE *out;
  char *tempstore;
  unsigned char randsuffix[9];

  if(!h)
    /* no cache activated */
    return CARLE_OK;

  /* if no new name is given, use the one we stored from the load */
  if(!file && h->filename)
    file = h->filename;

  if((h->flags & CARLHSTS_READONLYFILE) || !file || !file[0])
    /* marked as read-only, no file or zero length file name */
    goto skipsave;

  if(Curl_rand_hex(data, randsuffix, sizeof(randsuffix)))
    return CARLE_FAILED_INIT;

  tempstore = aprintf("%s.%s.tmp", file, randsuffix);
  if(!tempstore)
    return CARLE_OUT_OF_MEMORY;

  out = fopen(tempstore, FOPEN_WRITETEXT);
  if(!out)
    result = CARLE_WRITE_ERROR;
  else {
    fputs("# Your HSTS cache. https://carl.se/docs/hsts.html\n"
          "# This file was generated by libcarl! Edit at your own risk.\n",
          out);
    for(e = h->list.head; e; e = n) {
      struct stsentry *sts = e->ptr;
      n = e->next;
      result = hsts_out(sts, out);
      if(result)
        break;
    }
    fclose(out);
    if(!result && Curl_rename(tempstore, file))
      result = CARLE_WRITE_ERROR;

    if(result)
      unlink(tempstore);
  }
  free(tempstore);
  skipsave:
  if(data->set.hsts_write) {
    /* if there's a write callback */
    struct carl_index i; /* count */
    i.total = h->list.size;
    i.index = 0;
    for(e = h->list.head; e; e = n) {
      struct stsentry *sts = e->ptr;
      bool stop;
      n = e->next;
      result = hsts_push(data, &i, sts, &stop);
      if(result || stop)
        break;
      i.index++;
    }
  }
  return result;
}

/* only returns SERIOUS errors */
static CARLcode hsts_add(struct hsts *h, char *line)
{
  /* Example lines:
     example.com "20191231 10:00:00"
     .example.net "20191231 10:00:00"
   */
  char host[MAX_HSTS_HOSTLEN + 1];
  char date[MAX_HSTS_DATELEN + 1];
  int rc;

  rc = sscanf(line,
              "%" MAX_HSTS_HOSTLENSTR "s \"%" MAX_HSTS_DATELENSTR "[^\"]\"",
              host, date);
  if(2 == rc) {
    time_t expires = Curl_getdate_capped(date);
    CARLcode result;
    char *p = host;
    bool subdomain = FALSE;
    if(p[0] == '.') {
      p++;
      subdomain = TRUE;
    }
    result = hsts_create(h, p, subdomain, expires);
    if(result)
      return result;
  }

  return CARLE_OK;
}

/*
 * Load HSTS data from callback.
 *
 */
static CARLcode hsts_pull(struct Curl_easy *data, struct hsts *h)
{
  /* if the HSTS read callback is set, use it */
  if(data->set.hsts_read) {
    CARLSTScode sc;
    DEBUGASSERT(h);
    do {
      char buffer[257];
      struct carl_hstsentry e;
      e.name = buffer;
      e.namelen = sizeof(buffer)-1;
      e.includeSubDomains = FALSE; /* default */
      e.expire[0] = 0;
      e.name[0] = 0; /* just to make it clean */
      sc = data->set.hsts_read(data, &e, data->set.hsts_read_userp);
      if(sc == CARLSTS_OK) {
        time_t expires;
        CARLcode result;
        if(!e.name[0])
          /* bail out if no name was stored */
          return CARLE_BAD_FUNCTION_ARGUMENT;
        if(e.expire[0])
          expires = Curl_getdate_capped(e.expire);
        else
          expires = TIME_T_MAX; /* the end of time */
        result = hsts_create(h, e.name, e.includeSubDomains, expires);
        if(result)
          return result;
      }
      else if(sc == CARLSTS_FAIL)
        return CARLE_BAD_FUNCTION_ARGUMENT;
    } while(sc == CARLSTS_OK);
  }
  return CARLE_OK;
}

/*
 * Load the HSTS cache from the given file. The text based line-oriented file
 * format is documented here:
 * https://github.com/carl/carl/wiki/HSTS
 *
 * This function only returns error on major problems that prevent hsts
 * handling to work completely. It will ignore individual syntactical errors
 * etc.
 */
static CARLcode hsts_load(struct hsts *h, const char *file)
{
  CARLcode result = CARLE_OK;
  char *line = NULL;
  FILE *fp;

  /* we need a private copy of the file name so that the hsts cache file
     name survives an easy handle reset */
  free(h->filename);
  h->filename = strdup(file);
  if(!h->filename)
    return CARLE_OUT_OF_MEMORY;

  fp = fopen(file, FOPEN_READTEXT);
  if(fp) {
    line = malloc(MAX_HSTS_LINE);
    if(!line)
      goto fail;
    while(Curl_get_line(line, MAX_HSTS_LINE, fp)) {
      char *lineptr = line;
      while(*lineptr && ISBLANK(*lineptr))
        lineptr++;
      if(*lineptr == '#')
        /* skip commented lines */
        continue;

      hsts_add(h, lineptr);
    }
    free(line); /* free the line buffer */
    fclose(fp);
  }
  return result;

  fail:
  Curl_safefree(h->filename);
  fclose(fp);
  return CARLE_OUT_OF_MEMORY;
}

/*
 * Curl_hsts_loadfile() loads HSTS from file
 */
CARLcode Curl_hsts_loadfile(struct Curl_easy *data,
                            struct hsts *h, const char *file)
{
  DEBUGASSERT(h);
  (void)data;
  return hsts_load(h, file);
}

/*
 * Curl_hsts_loadcb() loads HSTS from callback
 */
CARLcode Curl_hsts_loadcb(struct Curl_easy *data, struct hsts *h)
{
  return hsts_pull(data, h);
}

#endif /* CARL_DISABLE_HTTP || USE_HSTS */
