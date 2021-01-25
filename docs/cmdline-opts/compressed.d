Long: compressed
Help: Request compressed response
Protocols: HTTP
Category: http
---
Request a compressed response using one of the algorithms carl supports, and
automatically decompress the content. Headers are not modified.

If this option is used and the server sends an unsupported encoding, carl will
report an error.
