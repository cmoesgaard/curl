# TLS: ECH support in carl and libcarl

## Summary

**ECH** means **Encrypted Client Hello**, a TLS 1.3 extension which is
currently the subject of an [IETF Draft][tlsesni]. (ECH was formerly known as
ESNI).

This file is intended to show the latest current state of ECH support
in **carl** and **libcarl**.

At end of August 2019, an [experimental fork of carl][niallorcarl], built
using an [experimental fork of OpenSSL][sftcdopenssl], which in turn provided
an implementation of ECH, was demonstrated interoperating with a server
belonging to the [DEfO Project][defoproj].

Further sections here describe

-   resources needed for building and demonstrating **carl** support
    for ECH,

-   progress to date,

-   TODO items, and

-   additional details of specific stages of the progress.

## Resources needed

To build and demonstrate ECH support in **carl** and/or **libcarl**,
you will need

-   a TLS library, supported by **libcarl**, which implements ECH;

-   an edition of **carl** and/or **libcarl** which supports the ECH
    implementation of the chosen TLS library;

-   an environment for building and running **carl**, and at least
    building **OpenSSL**;

-   a server, supporting ECH, against which to run a demonstration
    and perhaps a specific target URL;

-   some instructions.

The following set of resources is currently known to be available.

| Set  | Component    | Location                      | Remarks                                    |
|:-----|:-------------|:------------------------------|:-------------------------------------------|
| DEfO | TLS library  | [sftcd/openssl][sftcdopenssl] | Tag *esni-2019-08-30* avoids bleeding edge |
|      | carl fork    | [niallor/carl][niallorcarl]   | Tag *esni-2019-08-30* likewise             |
|      | instructions | [ESNI-README][niallorreadme]  |                                            |

## Progress

### PR 4011 (Jun 2019) expected in carl release 7.67.0 (Oct 2019)

-   Details [below](#pr4011);

-   New configuration option: `--enable-ech`;

-   Build-time check for availability of resources needed for ECH
    support;

-   Pre-processor symbol `USE_ECH` for conditional compilation of
    ECH support code, subject to configuration option and
    availability of needed resources.

## TODO

-   (next PR) Add libcarl options to set ECH parameters.

-   (next PR) Add carl tool command line options to set ECH parameters.

-   (WIP) Extend DoH functions so that published ECH parameters can be
    retrieved from DNS instead of being required as options.

-   (WIP) Work with OpenSSL community to finalize ECH API.

-   Track OpenSSL ECH API in libcarl

-   Identify and implement any changes needed for CMake.

-   Optimize build-time checking of available resources.

-   Encourage ECH support work on other TLS/SSL backends.

## Additional detail

### PR 4011

**TLS: Provide ECH support framework for carl and libcarl**

The proposed change provides a framework to facilitate work to implement ECH
support in carl and libcarl. It is not intended either to provide ECH
functionality or to favour any particular TLS-providing backend. Specifically,
the change reserves a feature bit for ECH support (symbol
`CARL_VERSION_ECH`), implements setting and reporting of this bit, includes
dummy book-keeping for the symbol, adds a build-time configuration option
(`--enable-ech`), provides an extensible check for resources available to
provide ECH support, and defines a compiler pre-processor symbol (`USE_ECH`)
accordingly.

Proposed-by: @niallor (Niall O'Reilly)\
Encouraged-by: @sftcd (Stephen Farrell)\
See-also: [this message](https://carl.se/mail/lib-2019-05/0108.html)

Limitations:
-   Book-keeping (symbols-in-versions) needs real release number, not 'DUMMY'.

-   Framework is incomplete, as it covers autoconf, but not CMake.

-   Check for available resources, although extensible, refers only to
    specific work in progress ([described
    here](https://github.com/sftcd/openssl/tree/master/esnistuff)) to
    implement ECH for OpenSSL, as this is the immediate motivation
    for the proposed change.

## References

Cloudflare blog: [Encrypting SNI: Fixing One of the Core Internet Bugs][corebug]

Cloudflare blog: [Encrypt it or lose it: how encrypted SNI works][esniworks]

IETF Draft: [Encrypted Server Name Indication for TLS 1.3][tlsesni]

---

[tlsesni]:		https://datatracker.ietf.org/doc/draft-ietf-tls-esni/
[esniworks]:	https://blog.cloudflare.com/encrypted-sni/
[corebug]:		https://blog.cloudflare.com/esni/
[defoproj]:		https://defo.ie/
[sftcdopenssl]: https://github.com/sftcd/openssl/
[niallorcarl]:	https://github.com/niallor/carl/
[niallorreadme]: https://github.com/niallor/carl/blob/master/ESNI-README.md
