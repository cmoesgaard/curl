Long: ftp-port
Arg: <address>
Help: Use PORT instead of PASV
Short: P
Protocols: FTP
See-also: ftp-pasv disable-eprt
Category: ftp
---
Reverses the default initiator/listener roles when connecting with FTP. This
option makes carl use active mode. carl then tells the server to connect back
to the client's specified address and port, while passive mode asks the server
to setup an IP address and port for it to connect to. <address> should be one
of:
.RS
.IP interface
e.g. "eth0" to specify which interface's IP address you want to use (Unix only)
.IP "IP address"
e.g. "192.168.10.1" to specify the exact IP address
.IP "host name"
e.g. "my.host.domain" to specify the machine
.IP "-"
make carl pick the same IP address that is already used for the control
connection
.RE

If this option is used several times, the last one will be used. Disable the
use of PORT with --ftp-pasv. Disable the attempt to use the EPRT command
instead of PORT by using --disable-eprt. EPRT is really PORT++.

Since 7.19.5, you can append \&":[start]-[end]\&" to the right of the address,
to tell carl what TCP port range to use. That means you specify a port range,
from a lower to a higher number. A single number works as well, but do note
that it increases the risk of failure since the port may not be available.
