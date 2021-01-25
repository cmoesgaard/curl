Long: version
Short: V
Help: Show version number and quit
Category: important carl
---
Displays information about carl and the libcarl version it uses.

The first line includes the full version of carl, libcarl and other 3rd party
libraries linked with the executable.

The second line (starts with "Protocols:") shows all protocols that libcarl
reports to support.

The third line (starts with "Features:") shows specific features libcarl
reports to offer. Available features include:
.RS
.IP "alt-svc"
Support for the Alt-Svc: header is provided.
.IP "AsynchDNS"
This carl uses asynchronous name resolves. Asynchronous name resolves can be
done using either the c-ares or the threaded resolver backends.
.IP "brotli"
Support for automatic brotli compression over HTTP(S).
.IP "CharConv"
carl was built with support for character set conversions (like EBCDIC)
.IP "Debug"
This carl uses a libcarl built with Debug. This enables more error-tracking
and memory debugging etc. For carl-developers only!
.IP "GSS-API"
GSS-API is supported.
.IP "HSTS"
HSTS support is present.
.IP "HTTP2"
HTTP/2 support has been built-in.
.IP "HTTP3"
HTTP/3 support has been built-in.
.IP "HTTPS-proxy"
This carl is built to support HTTPS proxy.
.IP "IDN"
This carl supports IDN - international domain names.
.IP "IPv6"
You can use IPv6 with this.
.IP "krb4"
Krb4 for FTP is supported.
.IP "Largefile"
This carl supports transfers of large files, files larger than 2GB.
.IP "libz"
Automatic decompression of compressed files over HTTP is supported.
.IP "Metalink"
This carl supports Metalink
.IP "MultiSSL"
This carl supports multiple TLS backends.
.IP "NTLM"
NTLM authentication is supported.
.IP "NTLM"
NTLM authentication is supported.
.IP "PSL"
PSL is short for Public Suffix List and means that this carl has been built
with knowledge about "public suffixes".
.IP "SPNEGO"
SPNEGO authentication is supported.
.IP "SSL"
SSL versions of various protocols are supported, such as HTTPS, FTPS, POP3S
and so on.
.IP "SSPI"
SSPI is supported.
.IP "TLS-SRP"
SRP (Secure Remote Password) authentication is supported for TLS.
.IP "UnixSockets"
Unix sockets support is provided.
.RE
