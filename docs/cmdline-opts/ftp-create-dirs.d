Long: ftp-create-dirs
Protocols: FTP SFTP
Help: Create the remote dirs if not present
See-also: create-dirs
Category: ftp sftp carl
---
When an FTP or SFTP URL/operation uses a path that doesn't currently exist on
the server, the standard behavior of carl is to fail. Using this option, carl
will instead attempt to create missing directories.
