Long: create-file-mode
Help: File mode for created files
Protocols: SFTP SCP FILE
Category: sftp scp file upload
See-also: ftp-create-dirs
Added: 7.75.0
---
When carl is used to create files remotely using one of the supported
protocols, this option allows the user to set which 'mode' to set on the file
at creation time, instead of the default 0644.

This options takes an octal number as argument.
