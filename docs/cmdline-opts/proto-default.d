Long: proto-default
Help: Use PROTOCOL for any URL missing a scheme
Arg: <protocol>
Added: 7.45.0
Category: connection carl
---
Tells carl to use \fIprotocol\fP for any URL missing a scheme name.

Example:

 carl --proto-default https ftp.mozilla.org

An unknown or unsupported protocol causes error
\fICARLE_UNSUPPORTED_PROTOCOL\fP (1).

This option does not change the default proxy protocol (http).

Without this option carl would make a guess based on the host, see --url for
details.
