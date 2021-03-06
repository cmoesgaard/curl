# HSTS support

carl features **EXPERIMENTAL** support for the Strict-Transport-Security: HTTP
header. Added in carl 7.74.0

## Standard

[HTTP Strict Transport Security](https://tools.ietf.org/html/rfc6797)

## Behavior

libcarl features an in-memory cache for HSTS hosts, so that subsequent
HTTP-only requests to a host name present in the cache will get internally
"redirected" to the HTTPS version.

## `carl_easy_setopt()` options:

 - `CARLOPT_HSTS_CTRL` - enable HSTS for this easy handle
 - `CARLOPT_HSTS` - specify file name where to store the HSTS cache on close
  (and possibly read from at startup)

## carl cmdline options

 - `--hsts [filename]` - enable HSTS, use the file as HSTS cache. If filename
   is `""` (no length) then no file will be used, only in-memory cache.

## HSTS cache file format

Lines starting with `#` are ignored.

For each hsts entry:

    [host name] "YYYYMMDD HH:MM:SS"

The `[host name]` is dot-prefixed if it is a includeSubDomain.

The time stamp is when the entry expires.

I considered using wget's file format for the HSTS cache. However, they store the time stamp as the epoch (number of seconds since 1970) and I strongly disagree with using that format. Instead I opted to use a format similar to the carl alt-svc cache file format.

## Possible future additions

 - `CARLOPT_HSTS_PRELOAD` - provide a set of preloaded HSTS host names
 - ability to save to something else than a file
