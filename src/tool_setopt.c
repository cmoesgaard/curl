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

#ifndef CARL_DISABLE_LIBCARL_OPTION

#define ENABLE_CARLX_PRINTF
/* use our own printf() functions */
#include "carlx.h"

#include "tool_cfgable.h"
#include "tool_easysrc.h"
#include "tool_setopt.h"
#include "tool_convert.h"
#include "tool_msgs.h"

#include "memdebug.h" /* keep this as LAST include */

/* Lookup tables for converting setopt values back to symbols */
/* For enums, values may be in any order. */
/* For bit masks, put combinations first, then single bits, */
/* and finally any "NONE" value. */

#define NV(e) {#e, e}
#define NV1(e, v) {#e, (v)}
#define NVEND {NULL, 0}         /* sentinel to mark end of list */

const struct NameValue setopt_nv_CARLPROXY[] = {
  NV(CARLPROXY_HTTP),
  NV(CARLPROXY_HTTP_1_0),
  NV(CARLPROXY_HTTPS),
  NV(CARLPROXY_SOCKS4),
  NV(CARLPROXY_SOCKS5),
  NV(CARLPROXY_SOCKS4A),
  NV(CARLPROXY_SOCKS5_HOSTNAME),
  NVEND,
};

const struct NameValue setopt_nv_CARL_SOCKS_PROXY[] = {
  NV(CARLPROXY_SOCKS4),
  NV(CARLPROXY_SOCKS5),
  NV(CARLPROXY_SOCKS4A),
  NV(CARLPROXY_SOCKS5_HOSTNAME),
  NVEND,
};

const struct NameValueUnsigned setopt_nv_CARLHSTS[] = {
  NV(CARLHSTS_ENABLE),
  NVEND,
};

const struct NameValueUnsigned setopt_nv_CARLAUTH[] = {
  NV(CARLAUTH_ANY),             /* combination */
  NV(CARLAUTH_ANYSAFE),         /* combination */
  NV(CARLAUTH_BASIC),
  NV(CARLAUTH_DIGEST),
  NV(CARLAUTH_GSSNEGOTIATE),
  NV(CARLAUTH_NTLM),
  NV(CARLAUTH_DIGEST_IE),
  NV(CARLAUTH_NTLM_WB),
  NV(CARLAUTH_ONLY),
  NV(CARLAUTH_NONE),
  NVEND,
};

const struct NameValue setopt_nv_CARL_HTTP_VERSION[] = {
  NV(CARL_HTTP_VERSION_NONE),
  NV(CARL_HTTP_VERSION_1_0),
  NV(CARL_HTTP_VERSION_1_1),
  NV(CARL_HTTP_VERSION_2_0),
  NV(CARL_HTTP_VERSION_2TLS),
  NV(CARL_HTTP_VERSION_3),
  NVEND,
};

const struct NameValue setopt_nv_CARL_SSLVERSION[] = {
  NV(CARL_SSLVERSION_DEFAULT),
  NV(CARL_SSLVERSION_TLSv1),
  NV(CARL_SSLVERSION_SSLv2),
  NV(CARL_SSLVERSION_SSLv3),
  NV(CARL_SSLVERSION_TLSv1_0),
  NV(CARL_SSLVERSION_TLSv1_1),
  NV(CARL_SSLVERSION_TLSv1_2),
  NV(CARL_SSLVERSION_TLSv1_3),
  NVEND,
};

const struct NameValue setopt_nv_CARL_TIMECOND[] = {
  NV(CARL_TIMECOND_IFMODSINCE),
  NV(CARL_TIMECOND_IFUNMODSINCE),
  NV(CARL_TIMECOND_LASTMOD),
  NV(CARL_TIMECOND_NONE),
  NVEND,
};

const struct NameValue setopt_nv_CARLFTPSSL_CCC[] = {
  NV(CARLFTPSSL_CCC_NONE),
  NV(CARLFTPSSL_CCC_PASSIVE),
  NV(CARLFTPSSL_CCC_ACTIVE),
  NVEND,
};

const struct NameValue setopt_nv_CARLUSESSL[] = {
  NV(CARLUSESSL_NONE),
  NV(CARLUSESSL_TRY),
  NV(CARLUSESSL_CONTROL),
  NV(CARLUSESSL_ALL),
  NVEND,
};

const struct NameValueUnsigned setopt_nv_CARLSSLOPT[] = {
  NV(CARLSSLOPT_ALLOW_BEAST),
  NV(CARLSSLOPT_NO_REVOKE),
  NV(CARLSSLOPT_NO_PARTIALCHAIN),
  NV(CARLSSLOPT_REVOKE_BEST_EFFORT),
  NV(CARLSSLOPT_NATIVE_CA),
  NVEND,
};

const struct NameValue setopt_nv_CARL_NETRC[] = {
  NV(CARL_NETRC_IGNORED),
  NV(CARL_NETRC_OPTIONAL),
  NV(CARL_NETRC_REQUIRED),
  NVEND,
};

/* These mappings essentially triplicated - see
 * tool_libinfo.c and tool_paramhlp.c */
const struct NameValue setopt_nv_CARLPROTO[] = {
  NV(CARLPROTO_ALL),            /* combination */
  NV(CARLPROTO_DICT),
  NV(CARLPROTO_FILE),
  NV(CARLPROTO_FTP),
  NV(CARLPROTO_FTPS),
  NV(CARLPROTO_GOPHER),
  NV(CARLPROTO_HTTP),
  NV(CARLPROTO_HTTPS),
  NV(CARLPROTO_IMAP),
  NV(CARLPROTO_IMAPS),
  NV(CARLPROTO_LDAP),
  NV(CARLPROTO_LDAPS),
  NV(CARLPROTO_POP3),
  NV(CARLPROTO_POP3S),
  NV(CARLPROTO_RTSP),
  NV(CARLPROTO_SCP),
  NV(CARLPROTO_SFTP),
  NV(CARLPROTO_SMB),
  NV(CARLPROTO_SMBS),
  NV(CARLPROTO_SMTP),
  NV(CARLPROTO_SMTPS),
  NV(CARLPROTO_TELNET),
  NV(CARLPROTO_TFTP),
  NVEND,
};

/* These options have non-zero default values. */
static const struct NameValue setopt_nv_CARLNONZERODEFAULTS[] = {
  NV1(CARLOPT_SSL_VERIFYPEER, 1),
  NV1(CARLOPT_SSL_VERIFYHOST, 1),
  NV1(CARLOPT_SSL_ENABLE_NPN, 1),
  NV1(CARLOPT_SSL_ENABLE_ALPN, 1),
  NV1(CARLOPT_TCP_NODELAY, 1),
  NV1(CARLOPT_PROXY_SSL_VERIFYPEER, 1),
  NV1(CARLOPT_PROXY_SSL_VERIFYHOST, 1),
  NV1(CARLOPT_SOCKS5_AUTH, 1),
  NVEND
};

/* Format and add code; jump to nomem on malloc error */
#define ADD(args) do { \
  ret = easysrc_add args; \
  if(ret) \
    goto nomem; \
} while(0)
#define ADDF(args) do { \
  ret = easysrc_addf args; \
  if(ret) \
    goto nomem; \
} while(0)
#define NULL_CHECK(p) do { \
  if(!p) { \
    ret = CARLE_OUT_OF_MEMORY; \
    goto nomem; \
  } \
} while(0)

#define DECL0(s) ADD((&easysrc_decl, s))
#define DECL1(f,a) ADDF((&easysrc_decl, f,a))

#define DATA0(s) ADD((&easysrc_data, s))
#define DATA1(f,a) ADDF((&easysrc_data, f,a))
#define DATA2(f,a,b) ADDF((&easysrc_data, f,a,b))
#define DATA3(f,a,b,c) ADDF((&easysrc_data, f,a,b,c))

#define CODE0(s) ADD((&easysrc_code, s))
#define CODE1(f,a) ADDF((&easysrc_code, f,a))
#define CODE2(f,a,b) ADDF((&easysrc_code, f,a,b))
#define CODE3(f,a,b,c) ADDF((&easysrc_code, f,a,b,c))

#define CLEAN0(s) ADD((&easysrc_clean, s))
#define CLEAN1(f,a) ADDF((&easysrc_clean, f,a))

#define REM0(s) ADD((&easysrc_toohard, s))
#define REM1(f,a) ADDF((&easysrc_toohard, f,a))
#define REM2(f,a,b) ADDF((&easysrc_toohard, f,a,b))

/* Escape string to C string syntax.  Return NULL if out of memory.
 * Is this correct for those wacky EBCDIC guys? */

#define MAX_STRING_LENGTH_OUTPUT 2000
#define ZERO_TERMINATED -1

static char *c_escape(const char *str, carl_off_t len)
{
  const char *s;
  unsigned char c;
  char *escaped, *e;
  unsigned int cutoff = 0;

  if(len == ZERO_TERMINATED)
    len = strlen(str);

  if(len > MAX_STRING_LENGTH_OUTPUT) {
    /* cap ridiculously long strings */
    len = MAX_STRING_LENGTH_OUTPUT;
    cutoff = 3;
  }

  /* Allocate space based on worst-case */
  escaped = malloc(4 * (size_t)len + 1 + cutoff);
  if(!escaped)
    return NULL;

  e = escaped;
  for(s = str; len; s++, len--) {
    c = *s;
    if(c == '\n') {
      strcpy(e, "\\n");
      e += 2;
    }
    else if(c == '\r') {
      strcpy(e, "\\r");
      e += 2;
    }
    else if(c == '\t') {
      strcpy(e, "\\t");
      e += 2;
    }
    else if(c == '\\') {
      strcpy(e, "\\\\");
      e += 2;
    }
    else if(c == '"') {
      strcpy(e, "\\\"");
      e += 2;
    }
    else if(!isprint(c)) {
      msnprintf(e, 5, "\\x%02x", (unsigned)c);
      e += 4;
    }
    else
      *e++ = c;
  }
  while(cutoff--)
    *e++ = '.';
  *e = '\0';
  return escaped;
}

/* setopt wrapper for enum types */
CARLcode tool_setopt_enum(CARL *carl, struct GlobalConfig *config,
                          const char *name, CARLoption tag,
                          const struct NameValue *nvlist, long lval)
{
  CARLcode ret = CARLE_OK;
  bool skip = FALSE;

  ret = carl_easy_setopt(carl, tag, lval);
  if(!lval)
    skip = TRUE;

  if(config->libcarl && !skip && !ret) {
    /* we only use this for real if --libcarl was used */
    const struct NameValue *nv = NULL;
    for(nv = nvlist; nv->name; nv++) {
      if(nv->value == lval)
        break; /* found it */
    }
    if(!nv->name) {
      /* If no definition was found, output an explicit value.
       * This could happen if new values are defined and used
       * but the NameValue list is not updated. */
      CODE2("carl_easy_setopt(hnd, %s, %ldL);", name, lval);
    }
    else {
      CODE2("carl_easy_setopt(hnd, %s, (long)%s);", name, nv->name);
    }
  }

#ifdef DEBUGBUILD
  if(ret)
    warnf(config, "option %s returned error (%d)\n", name, (int)ret);
#endif
  nomem:
  return ret;
}

/* setopt wrapper for flags */
CARLcode tool_setopt_flags(CARL *carl, struct GlobalConfig *config,
                           const char *name, CARLoption tag,
                           const struct NameValue *nvlist, long lval)
{
  CARLcode ret = CARLE_OK;
  bool skip = FALSE;

  ret = carl_easy_setopt(carl, tag, lval);
  if(!lval)
    skip = TRUE;

  if(config->libcarl && !skip && !ret) {
    /* we only use this for real if --libcarl was used */
    char preamble[80];          /* should accommodate any symbol name */
    long rest = lval;           /* bits not handled yet */
    const struct NameValue *nv = NULL;
    msnprintf(preamble, sizeof(preamble),
              "carl_easy_setopt(hnd, %s, ", name);
    for(nv = nvlist; nv->name; nv++) {
      if((nv->value & ~ rest) == 0) {
        /* all value flags contained in rest */
        rest &= ~ nv->value;    /* remove bits handled here */
        CODE3("%s(long)%s%s",
              preamble, nv->name, rest ? " |" : ");");
        if(!rest)
          break;                /* handled them all */
        /* replace with all spaces for continuation line */
        msnprintf(preamble, sizeof(preamble), "%*s", strlen(preamble), "");
      }
    }
    /* If any bits have no definition, output an explicit value.
     * This could happen if new bits are defined and used
     * but the NameValue list is not updated. */
    if(rest)
      CODE2("%s%ldL);", preamble, rest);
  }

 nomem:
  return ret;
}

/* setopt wrapper for bitmasks */
CARLcode tool_setopt_bitmask(CARL *carl, struct GlobalConfig *config,
                             const char *name, CARLoption tag,
                             const struct NameValueUnsigned *nvlist,
                             long lval)
{
  CARLcode ret = CARLE_OK;
  bool skip = FALSE;

  ret = carl_easy_setopt(carl, tag, lval);
  if(!lval)
    skip = TRUE;

  if(config->libcarl && !skip && !ret) {
    /* we only use this for real if --libcarl was used */
    char preamble[80];
    unsigned long rest = (unsigned long)lval;
    const struct NameValueUnsigned *nv = NULL;
    msnprintf(preamble, sizeof(preamble),
              "carl_easy_setopt(hnd, %s, ", name);
    for(nv = nvlist; nv->name; nv++) {
      if((nv->value & ~ rest) == 0) {
        /* all value flags contained in rest */
        rest &= ~ nv->value;    /* remove bits handled here */
        CODE3("%s(long)%s%s",
              preamble, nv->name, rest ? " |" : ");");
        if(!rest)
          break;                /* handled them all */
        /* replace with all spaces for continuation line */
        msnprintf(preamble, sizeof(preamble), "%*s", strlen(preamble), "");
      }
    }
    /* If any bits have no definition, output an explicit value.
     * This could happen if new bits are defined and used
     * but the NameValue list is not updated. */
    if(rest)
      CODE2("%s%luUL);", preamble, rest);
  }

 nomem:
  return ret;
}

/* Generate code for a struct carl_slist. */
static CARLcode libcarl_generate_slist(struct carl_slist *slist, int *slistno)
{
  CARLcode ret = CARLE_OK;
  char *escaped = NULL;

  /* May need several slist variables, so invent name */
  *slistno = ++easysrc_slist_count;

  DECL1("struct carl_slist *slist%d;", *slistno);
  DATA1("slist%d = NULL;", *slistno);
  CLEAN1("carl_slist_free_all(slist%d);", *slistno);
  CLEAN1("slist%d = NULL;", *slistno);
  for(; slist; slist = slist->next) {
    Curl_safefree(escaped);
    escaped = c_escape(slist->data, ZERO_TERMINATED);
    if(!escaped)
      return CARLE_OUT_OF_MEMORY;
    DATA3("slist%d = carl_slist_append(slist%d, \"%s\");",
                                       *slistno, *slistno, escaped);
  }

 nomem:
  Curl_safefree(escaped);
  return ret;
}

static CARLcode libcarl_generate_mime(CARL *carl,
                                      struct GlobalConfig *config,
                                      struct tool_mime *toolmime,
                                      int *mimeno);     /* Forward. */

/* Wrapper to generate source code for a mime part. */
static CARLcode libcarl_generate_mime_part(CARL *carl,
                                           struct GlobalConfig *config,
                                           struct tool_mime *part,
                                           int mimeno)
{
  CARLcode ret = CARLE_OK;
  int submimeno = 0;
  char *escaped = NULL;
  const char *data = NULL;
  const char *filename = part->filename;

  /* Parts are linked in reverse order. */
  if(part->prev) {
    ret = libcarl_generate_mime_part(carl, config, part->prev, mimeno);
    if(ret)
      return ret;
  }

  /* Create the part. */
  CODE2("part%d = carl_mime_addpart(mime%d);", mimeno, mimeno);

  switch(part->kind) {
  case TOOLMIME_PARTS:
    ret = libcarl_generate_mime(carl, config, part, &submimeno);
    if(!ret) {
      CODE2("carl_mime_subparts(part%d, mime%d);", mimeno, submimeno);
      CODE1("mime%d = NULL;", submimeno);   /* Avoid freeing in CLEAN. */
    }
    break;

  case TOOLMIME_DATA:
#ifdef CARL_DOES_CONVERSIONS
    /* Data will be set in ASCII, thus issue a comment with clear text. */
    escaped = c_escape(part->data, ZERO_TERMINATED);
    NULL_CHECK(escaped);
    CODE1("/* \"%s\" */", escaped);

    /* Our data is always textual: convert it to ASCII. */
    {
      size_t size = strlen(part->data);
      char *cp = malloc(size + 1);

      NULL_CHECK(cp);
      memcpy(cp, part->data, size + 1);
      ret = convert_to_network(cp, size);
      data = cp;
    }
#else
    data = part->data;
#endif
    if(!ret) {
      Curl_safefree(escaped);
      escaped = c_escape(data, ZERO_TERMINATED);
      NULL_CHECK(escaped);
      CODE2("carl_mime_data(part%d, \"%s\", CARL_ZERO_TERMINATED);",
                            mimeno, escaped);
    }
    break;

  case TOOLMIME_FILE:
  case TOOLMIME_FILEDATA:
    escaped = c_escape(part->data, ZERO_TERMINATED);
    NULL_CHECK(escaped);
    CODE2("carl_mime_filedata(part%d, \"%s\");", mimeno, escaped);
    if(part->kind == TOOLMIME_FILEDATA && !filename) {
      CODE1("carl_mime_filename(part%d, NULL);", mimeno);
    }
    break;

  case TOOLMIME_STDIN:
    if(!filename)
      filename = "-";
    /* FALLTHROUGH */
  case TOOLMIME_STDINDATA:
    /* Can only be reading stdin in the current context. */
    CODE1("carl_mime_data_cb(part%d, -1, (carl_read_callback) fread, \\",
          mimeno);
    CODE0("                  (carl_seek_callback) fseek, NULL, stdin);");
    break;
  default:
    /* Other cases not possible in this context. */
    break;
  }

  if(!ret && part->encoder) {
    Curl_safefree(escaped);
    escaped = c_escape(part->encoder, ZERO_TERMINATED);
    NULL_CHECK(escaped);
    CODE2("carl_mime_encoder(part%d, \"%s\");", mimeno, escaped);
  }

  if(!ret && filename) {
    Curl_safefree(escaped);
    escaped = c_escape(filename, ZERO_TERMINATED);
    NULL_CHECK(escaped);
    CODE2("carl_mime_filename(part%d, \"%s\");", mimeno, escaped);
  }

  if(!ret && part->name) {
    Curl_safefree(escaped);
    escaped = c_escape(part->name, ZERO_TERMINATED);
    NULL_CHECK(escaped);
    CODE2("carl_mime_name(part%d, \"%s\");", mimeno, escaped);
  }

  if(!ret && part->type) {
    Curl_safefree(escaped);
    escaped = c_escape(part->type, ZERO_TERMINATED);
    NULL_CHECK(escaped);
    CODE2("carl_mime_type(part%d, \"%s\");", mimeno, escaped);
  }

  if(!ret && part->headers) {
    int slistno;

    ret = libcarl_generate_slist(part->headers, &slistno);
    if(!ret) {
      CODE2("carl_mime_headers(part%d, slist%d, 1);", mimeno, slistno);
      CODE1("slist%d = NULL;", slistno); /* Prevent CLEANing. */
    }
  }

nomem:
#ifdef CARL_DOES_CONVERSIONS
  if(data)
    free((char *) data);
#endif

  Curl_safefree(escaped);
  return ret;
}

/* Wrapper to generate source code for a mime structure. */
static CARLcode libcarl_generate_mime(CARL *carl,
                                      struct GlobalConfig *config,
                                      struct tool_mime *toolmime,
                                      int *mimeno)
{
  CARLcode ret = CARLE_OK;

  /* May need several mime variables, so invent name. */
  *mimeno = ++easysrc_mime_count;
  DECL1("carl_mime *mime%d;", *mimeno);
  DATA1("mime%d = NULL;", *mimeno);
  CODE1("mime%d = carl_mime_init(hnd);", *mimeno);
  CLEAN1("carl_mime_free(mime%d);", *mimeno);
  CLEAN1("mime%d = NULL;", *mimeno);

  if(toolmime->subparts) {
    DECL1("carl_mimepart *part%d;", *mimeno);
    ret = libcarl_generate_mime_part(carl, config,
                                     toolmime->subparts, *mimeno);
  }

nomem:
  return ret;
}

/* setopt wrapper for CARLOPT_MIMEPOST */
CARLcode tool_setopt_mimepost(CARL *carl, struct GlobalConfig *config,
                              const char *name, CARLoption tag,
                              carl_mime *mimepost)
{
  CARLcode ret = carl_easy_setopt(carl, tag, mimepost);
  int mimeno = 0;

  if(!ret && config->libcarl) {
    ret = libcarl_generate_mime(carl, config,
                                config->current->mimeroot, &mimeno);

    if(!ret)
      CODE2("carl_easy_setopt(hnd, %s, mime%d);", name, mimeno);
  }

nomem:
  return ret;
}

/* setopt wrapper for carl_slist options */
CARLcode tool_setopt_slist(CARL *carl, struct GlobalConfig *config,
                           const char *name, CARLoption tag,
                           struct carl_slist *list)
{
  CARLcode ret = CARLE_OK;

  ret = carl_easy_setopt(carl, tag, list);

  if(config->libcarl && list && !ret) {
    int i;

    ret = libcarl_generate_slist(list, &i);
    if(!ret)
      CODE2("carl_easy_setopt(hnd, %s, slist%d);", name, i);
  }

 nomem:
  return ret;
}

/* generic setopt wrapper for all other options.
 * Some type information is encoded in the tag value. */
CARLcode tool_setopt(CARL *carl, bool str, struct GlobalConfig *global,
                     struct OperationConfig *config,
                     const char *name, CARLoption tag, ...)
{
  va_list arg;
  char buf[256];
  const char *value = NULL;
  bool remark = FALSE;
  bool skip = FALSE;
  bool escape = FALSE;
  char *escaped = NULL;
  CARLcode ret = CARLE_OK;

  va_start(arg, tag);

  if(tag < CARLOPTTYPE_OBJECTPOINT) {
    /* Value is expected to be a long */
    long lval = va_arg(arg, long);
    long defval = 0L;
    const struct NameValue *nv = NULL;
    for(nv = setopt_nv_CARLNONZERODEFAULTS; nv->name; nv++) {
      if(!strcmp(name, nv->name)) {
        defval = nv->value;
        break; /* found it */
      }
    }

    msnprintf(buf, sizeof(buf), "%ldL", lval);
    value = buf;
    ret = carl_easy_setopt(carl, tag, lval);
    if(lval == defval)
      skip = TRUE;
  }
  else if(tag < CARLOPTTYPE_OFF_T) {
    /* Value is some sort of object pointer */
    void *pval = va_arg(arg, void *);

    /* function pointers are never printable */
    if(tag >= CARLOPTTYPE_FUNCTIONPOINT) {
      if(pval) {
        value = "functionpointer";
        remark = TRUE;
      }
      else
        skip = TRUE;
    }

    else if(pval && str) {
      value = (char *)pval;
      escape = TRUE;
    }
    else if(pval) {
      value = "objectpointer";
      remark = TRUE;
    }
    else
      skip = TRUE;

    ret = carl_easy_setopt(carl, tag, pval);

  }
  else if(tag < CARLOPTTYPE_BLOB) {
    /* Value is expected to be carl_off_t */
    carl_off_t oval = va_arg(arg, carl_off_t);
    msnprintf(buf, sizeof(buf),
              "(carl_off_t)%" CARL_FORMAT_CARL_OFF_T, oval);
    value = buf;
    ret = carl_easy_setopt(carl, tag, oval);

    if(!oval)
      skip = TRUE;
  }
  else {
    /* Value is a blob */
    void *pblob = va_arg(arg, void *);

    /* blobs are never printable */
    if(pblob) {
      value = "blobpointer";
      remark = TRUE;
    }
    else
      skip = TRUE;

    ret = carl_easy_setopt(carl, tag, pblob);
  }

  va_end(arg);

  if(global->libcarl && !skip && !ret) {
    /* we only use this for real if --libcarl was used */

    if(remark)
      REM2("%s set to a %s", name, value);
    else {
      if(escape) {
        carl_off_t len = ZERO_TERMINATED;
        if(tag == CARLOPT_POSTFIELDS)
          len = config->postfieldsize;
        escaped = c_escape(value, len);
        NULL_CHECK(escaped);
        CODE2("carl_easy_setopt(hnd, %s, \"%s\");", name, escaped);
      }
      else
        CODE2("carl_easy_setopt(hnd, %s, %s);", name, value);
    }
  }

 nomem:
  Curl_safefree(escaped);
  return ret;
}

#else /* CARL_DISABLE_LIBCARL_OPTION */

#include "tool_cfgable.h"
#include "tool_setopt.h"

#endif /* CARL_DISABLE_LIBCARL_OPTION */

/*
 * tool_setopt_skip() allows the carl tool code to avoid setopt options that
 * are explicitly disabled in the build.
 */
bool tool_setopt_skip(CARLoption tag)
{
#ifdef CARL_DISABLE_PROXY
#define USED_TAG
  switch(tag) {
  case CARLOPT_HAPROXYPROTOCOL:
  case CARLOPT_HTTPPROXYTUNNEL:
  case CARLOPT_NOPROXY:
  case CARLOPT_PRE_PROXY:
  case CARLOPT_PROXY:
  case CARLOPT_PROXYAUTH:
  case CARLOPT_PROXY_CAINFO:
  case CARLOPT_PROXY_CAPATH:
  case CARLOPT_PROXY_CRLFILE:
  case CARLOPT_PROXYHEADER:
  case CARLOPT_PROXY_KEYPASSWD:
  case CARLOPT_PROXYPASSWORD:
  case CARLOPT_PROXY_PINNEDPUBLICKEY:
  case CARLOPT_PROXYPORT:
  case CARLOPT_PROXY_SERVICE_NAME:
  case CARLOPT_PROXY_SSLCERT:
  case CARLOPT_PROXY_SSLCERTTYPE:
  case CARLOPT_PROXY_SSL_CIPHER_LIST:
  case CARLOPT_PROXY_SSLKEY:
  case CARLOPT_PROXY_SSLKEYTYPE:
  case CARLOPT_PROXY_SSL_OPTIONS:
  case CARLOPT_PROXY_SSL_VERIFYHOST:
  case CARLOPT_PROXY_SSL_VERIFYPEER:
  case CARLOPT_PROXY_SSLVERSION:
  case CARLOPT_PROXY_TLS13_CIPHERS:
  case CARLOPT_PROXY_TLSAUTH_PASSWORD:
  case CARLOPT_PROXY_TLSAUTH_TYPE:
  case CARLOPT_PROXY_TLSAUTH_USERNAME:
  case CARLOPT_PROXY_TRANSFER_MODE:
  case CARLOPT_PROXYTYPE:
  case CARLOPT_PROXYUSERNAME:
  case CARLOPT_PROXYUSERPWD:
    return TRUE;
  default:
    break;
  }
#endif
#ifdef CARL_DISABLE_FTP
#define USED_TAG
  switch(tag) {
  case CARLOPT_FTPPORT:
  case CARLOPT_FTP_ACCOUNT:
  case CARLOPT_FTP_ALTERNATIVE_TO_USER:
  case CARLOPT_FTP_FILEMETHOD:
  case CARLOPT_FTP_SKIP_PASV_IP:
  case CARLOPT_FTP_USE_EPRT:
  case CARLOPT_FTP_USE_EPSV:
  case CARLOPT_FTP_USE_PRET:
  case CARLOPT_KRBLEVEL:
    return TRUE;
  default:
    break;
  }
#endif
#ifdef CARL_DISABLE_RTSP
#define USED_TAG
  switch(tag) {
  case CARLOPT_INTERLEAVEDATA:
    return TRUE;
  default:
    break;
  }
#endif
#if defined(CARL_DISABLE_HTTP) || defined(CARL_DISABLE_COOKIES)
#define USED_TAG
  switch(tag) {
  case CARLOPT_COOKIE:
  case CARLOPT_COOKIEFILE:
  case CARLOPT_COOKIEJAR:
  case CARLOPT_COOKIESESSION:
    return TRUE;
  default:
    break;
  }
#endif
#if defined(CARL_DISABLE_TELNET)
#define USED_TAG
  switch(tag) {
  case CARLOPT_TELNETOPTIONS:
    return TRUE;
  default:
    break;
  }
#endif
#ifdef CARL_DISABLE_TFTP
#define USED_TAG
  switch(tag) {
  case CARLOPT_TFTP_BLKSIZE:
  case CARLOPT_TFTP_NO_OPTIONS:
    return TRUE;
  default:
    break;
  }
#endif
#ifdef CARL_DISABLE_NETRC
#define USED_TAG
  switch(tag) {
  case CARLOPT_NETRC:
  case CARLOPT_NETRC_FILE:
    return TRUE;
  default:
    break;
  }
#endif

#ifndef USED_TAG
  (void)tag;
#endif
  return FALSE;
}
