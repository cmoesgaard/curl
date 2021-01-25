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
#include "tool_setup.h"
#if defined(HAVE_STRCASECMP) && defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include "tool_panykey.h"
#include "tool_help.h"
#include "tool_libinfo.h"
#include "tool_version.h"

#include "memdebug.h" /* keep this as LAST include */

#ifdef MSDOS
#  define USE_WATT32
#endif

/*
 * The bitmask output is generated with the following command
 ------------------------------------------------------------
  cd $srcroot/docs/cmdline-opts
  ./gen.pl listcats *.d
 */

#define CARLHELP_HIDDEN 1u << 0u
#define CARLHELP_AUTH 1u << 1u
#define CARLHELP_CONNECTION 1u << 2u
#define CARLHELP_CARL 1u << 3u
#define CARLHELP_DNS 1u << 4u
#define CARLHELP_FILE 1u << 5u
#define CARLHELP_FTP 1u << 6u
#define CARLHELP_HTTP 1u << 7u
#define CARLHELP_IMAP 1u << 8u
#define CARLHELP_IMPORTANT 1u << 9u
#define CARLHELP_MISC 1u << 10u
#define CARLHELP_OUTPUT 1u << 11u
#define CARLHELP_POP3 1u << 12u
#define CARLHELP_POST 1u << 13u
#define CARLHELP_PROXY 1u << 14u
#define CARLHELP_SCP 1u << 15u
#define CARLHELP_SFTP 1u << 16u
#define CARLHELP_SMTP 1u << 17u
#define CARLHELP_SSH 1u << 18u
#define CARLHELP_TELNET 1u << 19u
#define CARLHELP_TFTP 1u << 20u
#define CARLHELP_TLS 1u << 21u
#define CARLHELP_UPLOAD 1u << 22u
#define CARLHELP_VERBOSE 1u << 23u

typedef unsigned int carlhelp_t;

struct category_descriptors {
  const char *opt;
  const char *desc;
  carlhelp_t category;
};

static const struct category_descriptors categories[] = {
  {"auth", "Different types of authentication methods", CARLHELP_AUTH},
  {"connection", "Low level networking operations",
   CARLHELP_CONNECTION},
  {"carl", "The command line tool itself", CARLHELP_CARL},
  {"dns", "General DNS options", CARLHELP_DNS},
  {"file", "FILE protocol options", CARLHELP_FILE},
  {"ftp", "FTP protocol options", CARLHELP_FTP},
  {"http", "HTTP and HTTPS protocol options", CARLHELP_HTTP},
  {"imap", "IMAP protocol options", CARLHELP_IMAP},
  /* important is left out because it is the default help page */
  {"misc", "Options that don't fit into any other category", CARLHELP_MISC},
  {"output", "Filesystem output", CARLHELP_OUTPUT},
  {"pop3", "POP3 protocol options", CARLHELP_POP3},
  {"post", "HTTP Post specific options", CARLHELP_POST},
  {"proxy", "All options related to proxies", CARLHELP_PROXY},
  {"scp", "SCP protocol options", CARLHELP_SCP},
  {"sftp", "SFTP protocol options", CARLHELP_SFTP},
  {"smtp", "SMTP protocol options", CARLHELP_SMTP},
  {"ssh", "SSH protocol options", CARLHELP_SSH},
  {"telnet", "TELNET protocol options", CARLHELP_TELNET},
  {"tftp", "TFTP protocol options", CARLHELP_TFTP},
  {"tls", "All TLS/SSL related options", CARLHELP_TLS},
  {"upload", "All options for uploads",
   CARLHELP_UPLOAD},
  {"verbose", "Options related to any kind of command line output of carl",
   CARLHELP_VERBOSE},
  {NULL, NULL, CARLHELP_HIDDEN}
};

/*
 * The help output is generated with the following command
 ---------------------------------------------------------

  cd $srcroot/docs/cmdline-opts
  ./gen.pl listhelp *.d
 */

struct helptxt {
  const char *opt;
  const char *desc;
  carlhelp_t categories;
};


static const struct helptxt helptext[] = {
  {"    --abstract-unix-socket <path>",
   "Connect via abstract Unix domain socket",
   CARLHELP_CONNECTION},
  {"    --alt-svc <file name>",
   "Enable alt-svc with this cache file",
   CARLHELP_HTTP},
  {"    --anyauth",
   "Pick any authentication method",
   CARLHELP_HTTP | CARLHELP_PROXY | CARLHELP_AUTH},
  {"-a, --append",
   "Append to target file when uploading",
   CARLHELP_FTP | CARLHELP_SFTP},
  {"    --aws-sigv4 <provider1[:provider2]>",
   "Use AWS V4 signature authentication",
   CARLHELP_AUTH | CARLHELP_HTTP},
  {"    --basic",
   "Use HTTP Basic Authentication",
   CARLHELP_AUTH},
  {"    --cacert <file>",
   "CA certificate to verify peer against",
   CARLHELP_TLS},
  {"    --capath <dir>",
   "CA directory to verify peer against",
   CARLHELP_TLS},
  {"-E, --cert <certificate[:password]>",
   "Client certificate file and password",
   CARLHELP_TLS},
  {"    --cert-status",
   "Verify the status of the server certificate",
   CARLHELP_TLS},
  {"    --cert-type <type>",
   "Certificate type (DER/PEM/ENG)",
   CARLHELP_TLS},
  {"    --ciphers <list of ciphers>",
   "SSL ciphers to use",
   CARLHELP_TLS},
  {"    --compressed",
   "Request compressed response",
   CARLHELP_HTTP},
  {"    --compressed-ssh",
   "Enable SSH compression",
   CARLHELP_SCP | CARLHELP_SSH},
  {"-K, --config <file>",
   "Read config from a file",
   CARLHELP_CARL},
  {"    --connect-timeout <seconds>",
   "Maximum time allowed for connection",
   CARLHELP_CONNECTION},
  {"    --connect-to <HOST1:PORT1:HOST2:PORT2>",
   "Connect to host",
   CARLHELP_CONNECTION},
  {"-C, --continue-at <offset>",
   "Resumed transfer offset",
   CARLHELP_CONNECTION},
  {"-b, --cookie <data|filename>",
   "Send cookies from string/file",
   CARLHELP_HTTP},
  {"-c, --cookie-jar <filename>",
   "Write cookies to <filename> after operation",
   CARLHELP_HTTP},
  {"    --create-dirs",
   "Create necessary local directory hierarchy",
   CARLHELP_CARL},
  {"    --create-file-mode",
   "File mode for created files",
   CARLHELP_SFTP | CARLHELP_SCP | CARLHELP_FILE | CARLHELP_UPLOAD},
  {"    --crlf",
   "Convert LF to CRLF in upload",
   CARLHELP_FTP | CARLHELP_SMTP},
  {"    --crlfile <file>",
   "Get a CRL list in PEM format from the given file",
   CARLHELP_TLS},
  {"    --curves <algorithm list>",
   "(EC) TLS key exchange algorithm(s) to request",
   CARLHELP_TLS},
  {"-d, --data <data>",
   "HTTP POST data",
   CARLHELP_IMPORTANT | CARLHELP_HTTP | CARLHELP_POST | CARLHELP_UPLOAD},
  {"    --data-ascii <data>",
   "HTTP POST ASCII data",
   CARLHELP_HTTP | CARLHELP_POST | CARLHELP_UPLOAD},
  {"    --data-binary <data>",
   "HTTP POST binary data",
   CARLHELP_HTTP | CARLHELP_POST | CARLHELP_UPLOAD},
  {"    --data-raw <data>",
   "HTTP POST data, '@' allowed",
   CARLHELP_HTTP | CARLHELP_POST | CARLHELP_UPLOAD},
  {"    --data-urlencode <data>",
   "HTTP POST data url encoded",
   CARLHELP_HTTP | CARLHELP_POST | CARLHELP_UPLOAD},
  {"    --delegation <LEVEL>",
   "GSS-API delegation permission",
   CARLHELP_AUTH},
  {"    --digest",
   "Use HTTP Digest Authentication",
   CARLHELP_PROXY | CARLHELP_AUTH | CARLHELP_HTTP},
  {"-q, --disable",
   "Disable .carlrc",
   CARLHELP_CARL},
  {"    --disable-eprt",
   "Inhibit using EPRT or LPRT",
   CARLHELP_FTP},
  {"    --disable-epsv",
   "Inhibit using EPSV",
   CARLHELP_FTP},
  {"    --disallow-username-in-url",
   "Disallow username in url",
   CARLHELP_CARL | CARLHELP_HTTP},
  {"    --dns-interface <interface>",
   "Interface to use for DNS requests",
   CARLHELP_DNS},
  {"    --dns-ipv4-addr <address>",
   "IPv4 address to use for DNS requests",
   CARLHELP_DNS},
  {"    --dns-ipv6-addr <address>",
   "IPv6 address to use for DNS requests",
   CARLHELP_DNS},
  {"    --dns-servers <addresses>",
   "DNS server addrs to use",
   CARLHELP_DNS},
  {"    --doh-url <URL>",
   "Resolve host names over DOH",
   CARLHELP_DNS},
  {"-D, --dump-header <filename>",
   "Write the received headers to <filename>",
   CARLHELP_HTTP | CARLHELP_FTP},
  {"    --egd-file <file>",
   "EGD socket path for random data",
   CARLHELP_TLS},
  {"    --engine <name>",
   "Crypto engine to use",
   CARLHELP_TLS},
  {"    --etag-compare <file>",
   "Pass an ETag from a file as a custom header",
   CARLHELP_HTTP},
  {"    --etag-save <file>",
   "Parse ETag from a request and save it to a file",
   CARLHELP_HTTP},
  {"    --expect100-timeout <seconds>",
   "How long to wait for 100-continue",
   CARLHELP_HTTP},
  {"-f, --fail",
   "Fail silently (no output at all) on HTTP errors",
   CARLHELP_IMPORTANT | CARLHELP_HTTP},
  {"    --fail-early",
   "Fail on first transfer error, do not continue",
   CARLHELP_CARL},
  {"    --false-start",
   "Enable TLS False Start",
   CARLHELP_TLS},
  {"-F, --form <name=content>",
   "Specify multipart MIME data",
   CARLHELP_HTTP | CARLHELP_UPLOAD},
  {"    --form-string <name=string>",
   "Specify multipart MIME data",
   CARLHELP_HTTP | CARLHELP_UPLOAD},
  {"    --ftp-account <data>",
   "Account data string",
   CARLHELP_FTP | CARLHELP_AUTH},
  {"    --ftp-alternative-to-user <command>",
   "String to replace USER [name]",
   CARLHELP_FTP},
  {"    --ftp-create-dirs",
   "Create the remote dirs if not present",
   CARLHELP_FTP | CARLHELP_SFTP | CARLHELP_CARL},
  {"    --ftp-method <method>",
   "Control CWD usage",
   CARLHELP_FTP},
  {"    --ftp-pasv",
   "Use PASV/EPSV instead of PORT",
   CARLHELP_FTP},
  {"-P, --ftp-port <address>",
   "Use PORT instead of PASV",
   CARLHELP_FTP},
  {"    --ftp-pret",
   "Send PRET before PASV",
   CARLHELP_FTP},
  {"    --ftp-skip-pasv-ip",
   "Skip the IP address for PASV",
   CARLHELP_FTP},
  {"    --ftp-ssl-ccc",
   "Send CCC after authenticating",
   CARLHELP_FTP | CARLHELP_TLS},
  {"    --ftp-ssl-ccc-mode <active/passive>",
   "Set CCC mode",
   CARLHELP_FTP | CARLHELP_TLS},
  {"    --ftp-ssl-control",
   "Require SSL/TLS for FTP login, clear for transfer",
   CARLHELP_FTP | CARLHELP_TLS},
  {"-G, --get",
   "Put the post data in the URL and use GET",
   CARLHELP_HTTP | CARLHELP_UPLOAD},
  {"-g, --globoff",
   "Disable URL sequences and ranges using {} and []",
   CARLHELP_CARL},
  {"    --happy-eyeballs-timeout-ms <milliseconds>",
   "Time for IPv6 before trying IPv4",
   CARLHELP_CONNECTION},
  {"    --haproxy-protocol",
   "Send HAProxy PROXY protocol v1 header",
   CARLHELP_HTTP | CARLHELP_PROXY},
  {"-I, --head",
   "Show document info only",
   CARLHELP_HTTP | CARLHELP_FTP | CARLHELP_FILE},
  {"-H, --header <header/@file>",
   "Pass custom header(s) to server",
   CARLHELP_HTTP},
  {"-h, --help <category>",
   "Get help for commands",
   CARLHELP_IMPORTANT | CARLHELP_CARL},
  {"    --hostpubmd5 <md5>",
   "Acceptable MD5 hash of the host public key",
   CARLHELP_SFTP | CARLHELP_SCP},
  {"    --hsts <file name>",
   "Enable HSTS with this cache file",
   CARLHELP_HTTP},
  {"    --http0.9",
   "Allow HTTP 0.9 responses",
   CARLHELP_HTTP},
  {"-0, --http1.0",
   "Use HTTP 1.0",
   CARLHELP_HTTP},
  {"    --http1.1",
   "Use HTTP 1.1",
   CARLHELP_HTTP},
  {"    --http2",
   "Use HTTP 2",
   CARLHELP_HTTP},
  {"    --http2-prior-knowledge",
   "Use HTTP 2 without HTTP/1.1 Upgrade",
   CARLHELP_HTTP},
  {"    --http3",
   "Use HTTP v3",
   CARLHELP_HTTP},
  {"    --ignore-content-length",
   "Ignore the size of the remote resource",
   CARLHELP_HTTP | CARLHELP_FTP},
  {"-i, --include",
   "Include protocol response headers in the output",
   CARLHELP_IMPORTANT | CARLHELP_VERBOSE},
  {"-k, --insecure",
   "Allow insecure server connections when using SSL",
   CARLHELP_TLS},
  {"    --interface <name>",
   "Use network INTERFACE (or address)",
   CARLHELP_CONNECTION},
  {"-4, --ipv4",
   "Resolve names to IPv4 addresses",
   CARLHELP_CONNECTION | CARLHELP_DNS},
  {"-6, --ipv6",
   "Resolve names to IPv6 addresses",
   CARLHELP_CONNECTION | CARLHELP_DNS},
  {"-j, --junk-session-cookies",
   "Ignore session cookies read from file",
   CARLHELP_HTTP},
  {"    --keepalive-time <seconds>",
   "Interval time for keepalive probes",
   CARLHELP_CONNECTION},
  {"    --key <key>",
   "Private key file name",
   CARLHELP_TLS | CARLHELP_SSH},
  {"    --key-type <type>",
   "Private key file type (DER/PEM/ENG)",
   CARLHELP_TLS},
  {"    --krb <level>",
   "Enable Kerberos with security <level>",
   CARLHELP_FTP},
  {"    --libcarl <file>",
   "Dump libcarl equivalent code of this command line",
   CARLHELP_CARL},
  {"    --limit-rate <speed>",
   "Limit transfer speed to RATE",
   CARLHELP_CONNECTION},
  {"-l, --list-only",
   "List only mode",
   CARLHELP_FTP | CARLHELP_POP3},
  {"    --local-port <num/range>",
   "Force use of RANGE for local port numbers",
   CARLHELP_CONNECTION},
  {"-L, --location",
   "Follow redirects",
   CARLHELP_HTTP},
  {"    --location-trusted",
   "Like --location, and send auth to other hosts",
   CARLHELP_HTTP | CARLHELP_AUTH},
  {"    --login-options <options>",
   "Server login options",
   CARLHELP_IMAP | CARLHELP_POP3 | CARLHELP_SMTP | CARLHELP_AUTH},
  {"    --mail-auth <address>",
   "Originator address of the original email",
   CARLHELP_SMTP},
  {"    --mail-from <address>",
   "Mail from this address",
   CARLHELP_SMTP},
  {"    --mail-rcpt <address>",
   "Mail to this address",
   CARLHELP_SMTP},
  {"    --mail-rcpt-allowfails",
   "Allow RCPT TO command to fail for some recipients",
   CARLHELP_SMTP},
  {"-M, --manual",
   "Display the full manual",
   CARLHELP_CARL},
  {"    --max-filesize <bytes>",
   "Maximum file size to download",
   CARLHELP_CONNECTION},
  {"    --max-redirs <num>",
   "Maximum number of redirects allowed",
   CARLHELP_HTTP},
  {"-m, --max-time <seconds>",
   "Maximum time allowed for the transfer",
   CARLHELP_CONNECTION},
  {"    --metalink",
   "Process given URLs as metalink XML file",
   CARLHELP_MISC},
  {"    --negotiate",
   "Use HTTP Negotiate (SPNEGO) authentication",
   CARLHELP_AUTH | CARLHELP_HTTP},
  {"-n, --netrc",
   "Must read .netrc for user name and password",
   CARLHELP_CARL},
  {"    --netrc-file <filename>",
   "Specify FILE for netrc",
   CARLHELP_CARL},
  {"    --netrc-optional",
   "Use either .netrc or URL",
   CARLHELP_CARL},
  {"-:, --next",
   "Make next URL use its separate set of options",
   CARLHELP_CARL},
  {"    --no-alpn",
   "Disable the ALPN TLS extension",
   CARLHELP_TLS | CARLHELP_HTTP},
  {"-N, --no-buffer",
   "Disable buffering of the output stream",
   CARLHELP_CARL},
  {"    --no-keepalive",
   "Disable TCP keepalive on the connection",
   CARLHELP_CONNECTION},
  {"    --no-npn",
   "Disable the NPN TLS extension",
   CARLHELP_TLS | CARLHELP_HTTP},
  {"    --no-progress-meter",
   "Do not show the progress meter",
   CARLHELP_VERBOSE},
  {"    --no-sessionid",
   "Disable SSL session-ID reusing",
   CARLHELP_TLS},
  {"    --noproxy <no-proxy-list>",
   "List of hosts which do not use proxy",
   CARLHELP_PROXY},
  {"    --ntlm",
   "Use HTTP NTLM authentication",
   CARLHELP_AUTH | CARLHELP_HTTP},
  {"    --ntlm-wb",
   "Use HTTP NTLM authentication with winbind",
   CARLHELP_AUTH | CARLHELP_HTTP},
  {"    --oauth2-bearer <token>",
   "OAuth 2 Bearer Token",
   CARLHELP_AUTH},
  {"-o, --output <file>",
   "Write to file instead of stdout",
   CARLHELP_IMPORTANT | CARLHELP_CARL},
  {"    --output-dir <dir>",
   "Directory to save files in",
   CARLHELP_CARL},
  {"-Z, --parallel",
   "Perform transfers in parallel",
   CARLHELP_CONNECTION | CARLHELP_CARL},
  {"    --parallel-immediate",
   "Do not wait for multiplexing (with --parallel)",
   CARLHELP_CONNECTION | CARLHELP_CARL},
  {"    --parallel-max",
   "Maximum concurrency for parallel transfers",
   CARLHELP_CONNECTION | CARLHELP_CARL},
  {"    --pass <phrase>",
   "Pass phrase for the private key",
   CARLHELP_SSH | CARLHELP_TLS | CARLHELP_AUTH},
  {"    --path-as-is",
   "Do not squash .. sequences in URL path",
   CARLHELP_CARL},
  {"    --pinnedpubkey <hashes>",
   "FILE/HASHES Public key to verify peer against",
   CARLHELP_TLS},
  {"    --post301",
   "Do not switch to GET after following a 301",
   CARLHELP_HTTP | CARLHELP_POST},
  {"    --post302",
   "Do not switch to GET after following a 302",
   CARLHELP_HTTP | CARLHELP_POST},
  {"    --post303",
   "Do not switch to GET after following a 303",
   CARLHELP_HTTP | CARLHELP_POST},
  {"    --preproxy [protocol://]host[:port]",
   "Use this proxy first",
   CARLHELP_PROXY},
  {"-#, --progress-bar",
   "Display transfer progress as a bar",
   CARLHELP_VERBOSE},
  {"    --proto <protocols>",
   "Enable/disable PROTOCOLS",
   CARLHELP_CONNECTION | CARLHELP_CARL},
  {"    --proto-default <protocol>",
   "Use PROTOCOL for any URL missing a scheme",
   CARLHELP_CONNECTION | CARLHELP_CARL},
  {"    --proto-redir <protocols>",
   "Enable/disable PROTOCOLS on redirect",
   CARLHELP_CONNECTION | CARLHELP_CARL},
  {"-x, --proxy [protocol://]host[:port]",
   "Use this proxy",
   CARLHELP_PROXY},
  {"    --proxy-anyauth",
   "Pick any proxy authentication method",
   CARLHELP_PROXY | CARLHELP_AUTH},
  {"    --proxy-basic",
   "Use Basic authentication on the proxy",
   CARLHELP_PROXY | CARLHELP_AUTH},
  {"    --proxy-cacert <file>",
   "CA certificate to verify peer against for proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-capath <dir>",
   "CA directory to verify peer against for proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-cert <cert[:passwd]>",
   "Set client certificate for proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-cert-type <type>",
   "Client certificate type for HTTPS proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-ciphers <list>",
   "SSL ciphers to use for proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-crlfile <file>",
   "Set a CRL list for proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-digest",
   "Use Digest authentication on the proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-header <header/@file>",
   "Pass custom header(s) to proxy",
   CARLHELP_PROXY},
  {"    --proxy-insecure",
   "Do HTTPS proxy connections without verifying the proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-key <key>",
   "Private key for HTTPS proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-key-type <type>",
   "Private key file type for proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-negotiate",
   "Use HTTP Negotiate (SPNEGO) authentication on the proxy",
   CARLHELP_PROXY | CARLHELP_AUTH},
  {"    --proxy-ntlm",
   "Use NTLM authentication on the proxy",
   CARLHELP_PROXY | CARLHELP_AUTH},
  {"    --proxy-pass <phrase>",
   "Pass phrase for the private key for HTTPS proxy",
   CARLHELP_PROXY | CARLHELP_TLS | CARLHELP_AUTH},
  {"    --proxy-pinnedpubkey <hashes>",
   "FILE/HASHES public key to verify proxy with",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-service-name <name>",
   "SPNEGO proxy service name",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-ssl-allow-beast",
   "Allow security flaw for interop for HTTPS proxy",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-tls13-ciphers <ciphersuite list>",
   "TLS 1.3 proxy cipher suites",
   CARLHELP_PROXY | CARLHELP_TLS},
  {"    --proxy-tlsauthtype <type>",
   "TLS authentication type for HTTPS proxy",
   CARLHELP_PROXY | CARLHELP_TLS | CARLHELP_AUTH},
  {"    --proxy-tlspassword <string>",
   "TLS password for HTTPS proxy",
   CARLHELP_PROXY | CARLHELP_TLS | CARLHELP_AUTH},
  {"    --proxy-tlsuser <name>",
   "TLS username for HTTPS proxy",
   CARLHELP_PROXY | CARLHELP_TLS | CARLHELP_AUTH},
  {"    --proxy-tlsv1",
   "Use TLSv1 for HTTPS proxy",
   CARLHELP_PROXY | CARLHELP_TLS | CARLHELP_AUTH},
  {"-U, --proxy-user <user:password>",
   "Proxy user and password",
   CARLHELP_PROXY | CARLHELP_AUTH},
  {"    --proxy1.0 <host[:port]>",
   "Use HTTP/1.0 proxy on given port",
   CARLHELP_PROXY},
  {"-p, --proxytunnel",
   "Operate through an HTTP proxy tunnel (using CONNECT)",
   CARLHELP_PROXY},
  {"    --pubkey <key>",
   "SSH Public key file name",
   CARLHELP_SFTP | CARLHELP_SCP | CARLHELP_AUTH},
  {"-Q, --quote",
   "Send command(s) to server before transfer",
   CARLHELP_FTP | CARLHELP_SFTP},
  {"    --random-file <file>",
   "File for reading random data from",
   CARLHELP_MISC},
  {"-r, --range <range>",
   "Retrieve only the bytes within RANGE",
   CARLHELP_HTTP | CARLHELP_FTP | CARLHELP_SFTP | CARLHELP_FILE},
  {"    --raw",
   "Do HTTP \"raw\"; no transfer decoding",
   CARLHELP_HTTP},
  {"-e, --referer <URL>",
   "Referrer URL",
   CARLHELP_HTTP},
  {"-J, --remote-header-name",
   "Use the header-provided filename",
   CARLHELP_OUTPUT},
  {"-O, --remote-name",
   "Write output to a file named as the remote file",
   CARLHELP_IMPORTANT | CARLHELP_OUTPUT},
  {"    --remote-name-all",
   "Use the remote file name for all URLs",
   CARLHELP_OUTPUT},
  {"-R, --remote-time",
   "Set the remote file's time on the local output",
   CARLHELP_OUTPUT},
  {"-X, --request <command>",
   "Specify request command to use",
   CARLHELP_CONNECTION},
  {"    --request-target",
   "Specify the target for this request",
   CARLHELP_HTTP},
  {"    --resolve <[+]host:port:addr[,addr]...>",
   "Resolve the host+port to this address",
   CARLHELP_CONNECTION},
  {"    --retry <num>",
   "Retry request if transient problems occur",
   CARLHELP_CARL},
  {"    --retry-all-errors",
   "Retry all errors (use with --retry)",
   CARLHELP_CARL},
  {"    --retry-connrefused",
   "Retry on connection refused (use with --retry)",
   CARLHELP_CARL},
  {"    --retry-delay <seconds>",
   "Wait time between retries",
   CARLHELP_CARL},
  {"    --retry-max-time <seconds>",
   "Retry only within this period",
   CARLHELP_CARL},
  {"    --sasl-authzid <identity>",
   "Identity for SASL PLAIN authentication",
   CARLHELP_AUTH},
  {"    --sasl-ir",
   "Enable initial response in SASL authentication",
   CARLHELP_AUTH},
  {"    --service-name <name>",
   "SPNEGO service name",
   CARLHELP_MISC},
  {"-S, --show-error",
   "Show error even when -s is used",
   CARLHELP_CARL},
  {"-s, --silent",
   "Silent mode",
   CARLHELP_IMPORTANT | CARLHELP_VERBOSE},
  {"    --socks4 <host[:port]>",
   "SOCKS4 proxy on given host + port",
   CARLHELP_PROXY},
  {"    --socks4a <host[:port]>",
   "SOCKS4a proxy on given host + port",
   CARLHELP_PROXY},
  {"    --socks5 <host[:port]>",
   "SOCKS5 proxy on given host + port",
   CARLHELP_PROXY},
  {"    --socks5-basic",
   "Enable username/password auth for SOCKS5 proxies",
   CARLHELP_PROXY | CARLHELP_AUTH},
  {"    --socks5-gssapi",
   "Enable GSS-API auth for SOCKS5 proxies",
   CARLHELP_PROXY | CARLHELP_AUTH},
  {"    --socks5-gssapi-nec",
   "Compatibility with NEC SOCKS5 server",
   CARLHELP_PROXY | CARLHELP_AUTH},
  {"    --socks5-gssapi-service <name>",
   "SOCKS5 proxy service name for GSS-API",
   CARLHELP_PROXY | CARLHELP_AUTH},
  {"    --socks5-hostname <host[:port]>",
   "SOCKS5 proxy, pass host name to proxy",
   CARLHELP_PROXY},
  {"-Y, --speed-limit <speed>",
   "Stop transfers slower than this",
   CARLHELP_CONNECTION},
  {"-y, --speed-time <seconds>",
   "Trigger 'speed-limit' abort after this time",
   CARLHELP_CONNECTION},
  {"    --ssl",
   "Try SSL/TLS",
   CARLHELP_TLS},
  {"    --ssl-allow-beast",
   "Allow security flaw to improve interop",
   CARLHELP_TLS},
  {"    --ssl-no-revoke",
   "Disable cert revocation checks (Schannel)",
   CARLHELP_TLS},
  {"    --ssl-reqd",
   "Require SSL/TLS",
   CARLHELP_TLS},
  {"    --ssl-revoke-best-effort",
   "Ignore missing/offline cert CRL dist points",
   CARLHELP_TLS},
  {"-2, --sslv2",
   "Use SSLv2",
   CARLHELP_TLS},
  {"-3, --sslv3",
   "Use SSLv3",
   CARLHELP_TLS},
  {"    --stderr",
   "Where to redirect stderr",
   CARLHELP_VERBOSE},
  {"    --styled-output",
   "Enable styled output for HTTP headers",
   CARLHELP_VERBOSE},
  {"    --suppress-connect-headers",
   "Suppress proxy CONNECT response headers",
   CARLHELP_PROXY},
  {"    --tcp-fastopen",
   "Use TCP Fast Open",
   CARLHELP_CONNECTION},
  {"    --tcp-nodelay",
   "Use the TCP_NODELAY option",
   CARLHELP_CONNECTION},
  {"-t, --telnet-option <opt=val>",
   "Set telnet option",
   CARLHELP_TELNET},
  {"    --tftp-blksize <value>",
   "Set TFTP BLKSIZE option",
   CARLHELP_TFTP},
  {"    --tftp-no-options",
   "Do not send any TFTP options",
   CARLHELP_TFTP},
  {"-z, --time-cond <time>",
   "Transfer based on a time condition",
   CARLHELP_HTTP | CARLHELP_FTP},
  {"    --tls-max <VERSION>",
   "Set maximum allowed TLS version",
   CARLHELP_TLS},
  {"    --tls13-ciphers <ciphersuite list>",
   "TLS 1.3 cipher suites to use",
   CARLHELP_TLS},
  {"    --tlsauthtype <type>",
   "TLS authentication type",
   CARLHELP_TLS | CARLHELP_AUTH},
  {"    --tlspassword",
   "TLS password",
   CARLHELP_TLS | CARLHELP_AUTH},
  {"    --tlsuser <name>",
   "TLS user name",
   CARLHELP_TLS | CARLHELP_AUTH},
  {"-1, --tlsv1",
   "Use TLSv1.0 or greater",
   CARLHELP_TLS},
  {"    --tlsv1.0",
   "Use TLSv1.0 or greater",
   CARLHELP_TLS},
  {"    --tlsv1.1",
   "Use TLSv1.1 or greater",
   CARLHELP_TLS},
  {"    --tlsv1.2",
   "Use TLSv1.2 or greater",
   CARLHELP_TLS},
  {"    --tlsv1.3",
   "Use TLSv1.3 or greater",
   CARLHELP_TLS},
  {"    --tr-encoding",
   "Request compressed transfer encoding",
   CARLHELP_HTTP},
  {"    --trace <file>",
   "Write a debug trace to FILE",
   CARLHELP_VERBOSE},
  {"    --trace-ascii <file>",
   "Like --trace, but without hex output",
   CARLHELP_VERBOSE},
  {"    --trace-time",
   "Add time stamps to trace/verbose output",
   CARLHELP_VERBOSE},
  {"    --unix-socket <path>",
   "Connect through this Unix domain socket",
   CARLHELP_CONNECTION},
  {"-T, --upload-file <file>",
   "Transfer local FILE to destination",
   CARLHELP_IMPORTANT | CARLHELP_UPLOAD},
  {"    --url <url>",
   "URL to work with",
   CARLHELP_CARL},
  {"-B, --use-ascii",
   "Use ASCII/text transfer",
   CARLHELP_MISC},
  {"-u, --user <user:password>",
   "Server user and password",
   CARLHELP_IMPORTANT | CARLHELP_AUTH},
  {"-A, --user-agent <name>",
   "Send User-Agent <name> to server",
   CARLHELP_IMPORTANT | CARLHELP_HTTP},
  {"-v, --verbose",
   "Make the operation more talkative",
   CARLHELP_IMPORTANT | CARLHELP_VERBOSE},
  {"-V, --version",
   "Show version number and quit",
   CARLHELP_IMPORTANT | CARLHELP_CARL},
  {"-w, --write-out <format>",
   "Use output FORMAT after completion",
   CARLHELP_VERBOSE},
  {"    --xattr",
   "Store metadata in extended file attributes",
   CARLHELP_MISC},
  { NULL, NULL, CARLHELP_HIDDEN }
};

#ifdef NETWARE
#  define PRINT_LINES_PAUSE 23
#endif

struct feat {
  const char *name;
  int bitmask;
};

static const struct feat feats[] = {
  {"AsynchDNS",      CARL_VERSION_ASYNCHDNS},
  {"Debug",          CARL_VERSION_DEBUG},
  {"TrackMemory",    CARL_VERSION_CARLDEBUG},
  {"IDN",            CARL_VERSION_IDN},
  {"IPv6",           CARL_VERSION_IPV6},
  {"Largefile",      CARL_VERSION_LARGEFILE},
  {"Unicode",        CARL_VERSION_UNICODE},
  {"SSPI",           CARL_VERSION_SSPI},
  {"GSS-API",        CARL_VERSION_GSSAPI},
  {"Kerberos",       CARL_VERSION_KERBEROS5},
  {"SPNEGO",         CARL_VERSION_SPNEGO},
  {"NTLM",           CARL_VERSION_NTLM},
  {"NTLM_WB",        CARL_VERSION_NTLM_WB},
  {"SSL",            CARL_VERSION_SSL},
  {"libz",           CARL_VERSION_LIBZ},
  {"brotli",         CARL_VERSION_BROTLI},
  {"zstd",           CARL_VERSION_ZSTD},
  {"CharConv",       CARL_VERSION_CONV},
  {"TLS-SRP",        CARL_VERSION_TLSAUTH_SRP},
  {"HTTP2",          CARL_VERSION_HTTP2},
  {"HTTP3",          CARL_VERSION_HTTP3},
  {"UnixSockets",    CARL_VERSION_UNIX_SOCKETS},
  {"HTTPS-proxy",    CARL_VERSION_HTTPS_PROXY},
  {"MultiSSL",       CARL_VERSION_MULTI_SSL},
  {"PSL",            CARL_VERSION_PSL},
  {"alt-svc",        CARL_VERSION_ALTSVC},
  {"HSTS",           CARL_VERSION_HSTS},
};

static void print_category(carlhelp_t category)
{
  unsigned int i;
  for(i = 0; helptext[i].opt; ++i)
    if(helptext[i].categories & category) {
      printf(" %-19s %s\n", helptext[i].opt, helptext[i].desc);
    }
}

/* Prints category if found. If not, it returns 1 */
static int get_category_content(const char *category)
{
  unsigned int i;
  for(i = 0; categories[i].opt; ++i)
    if(carl_strequal(categories[i].opt, category)) {
      printf("%s: %s\n", categories[i].opt, categories[i].desc);
      print_category(categories[i].category);
      return 0;
    }
  return 1;
}

/* Prints all categories and their description */
static void get_categories(void)
{
  unsigned int i;
  for(i = 0; categories[i].opt; ++i)
    printf(" %-11s %s\n", categories[i].opt, categories[i].desc);
}


void tool_help(char *category)
{
  puts("Usage: carl [options...] <url>");
  /* If no category was provided */
  if(!category) {
    const char *category_note = "\nThis is not the full help, this "
      "menu is stripped into categories.\nUse \"--help category\" to get "
      "an overview of all categories.\nFor all options use the manual"
      " or \"--help all\".";
    print_category(CARLHELP_IMPORTANT);
    puts(category_note);
  }
  /* Lets print everything if "all" was provided */
  else if(carl_strequal(category, "all"))
    /* Print everything except hidden */
    print_category(~(CARLHELP_HIDDEN));
  /* Lets handle the string "category" differently to not print an errormsg */
  else if(carl_strequal(category, "category"))
    get_categories();
  /* Otherwise print category and handle the case if the cat was not found */
  else if(get_category_content(category)) {
    puts("Invalid category provided, here is a list of all categories:\n");
    get_categories();
  }
  free(category);
}

static int
featcomp(const void *p1, const void *p2)
{
  /* The arguments to this function are "pointers to pointers to char", but
     the comparison arguments are "pointers to char", hence the following cast
     plus dereference */
#ifdef HAVE_STRCASECMP
  return strcasecmp(* (char * const *) p1, * (char * const *) p2);
#elif defined(HAVE_STRCMPI)
  return strcmpi(* (char * const *) p1, * (char * const *) p2);
#else
  return strcmp(* (char * const *) p1, * (char * const *) p2);
#endif
}

void tool_version_info(void)
{
  const char *const *proto;

  printf(CARL_ID "%s\n", carl_version());
#ifdef CARL_PATCHSTAMP
  printf("Release-Date: %s, security patched: %s\n",
         LIBCARL_TIMESTAMP, CARL_PATCHSTAMP);
#else
  printf("Release-Date: %s\n", LIBCARL_TIMESTAMP);
#endif
  if(carlinfo->protocols) {
    printf("Protocols: ");
    for(proto = carlinfo->protocols; *proto; ++proto) {
      printf("%s ", *proto);
    }
    puts(""); /* newline */
  }
  if(carlinfo->features) {
    char *featp[ sizeof(feats) / sizeof(feats[0]) + 1];
    size_t numfeat = 0;
    unsigned int i;
    printf("Features:");
    for(i = 0; i < sizeof(feats)/sizeof(feats[0]); i++) {
      if(carlinfo->features & feats[i].bitmask)
        featp[numfeat++] = (char *)feats[i].name;
    }
#ifdef USE_METALINK
    featp[numfeat++] = (char *)"Metalink";
#endif
    qsort(&featp[0], numfeat, sizeof(char *), featcomp);
    for(i = 0; i< numfeat; i++)
      printf(" %s", featp[i]);
    puts(""); /* newline */
  }
  if(strcmp(CARL_VERSION, carlinfo->version)) {
    printf("WARNING: carl and libcarl versions do not match. "
           "Functionality may be affected.\n");
  }
}

void tool_list_engines(void)
{
  CARL *carl = carl_easy_init();
  struct carl_slist *engines = NULL;

  /* Get the list of engines */
  carl_easy_getinfo(carl, CARLINFO_SSL_ENGINES, &engines);

  puts("Build-time engines:");
  if(engines) {
    for(; engines; engines = engines->next)
      printf("  %s\n", engines->data);
  }
  else {
    puts("  <none>");
  }

  /* Cleanup the list of engines */
  carl_slist_free_all(engines);
  carl_easy_cleanup(carl);
}
