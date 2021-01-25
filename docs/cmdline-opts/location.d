Long: location
Short: L
Help: Follow redirects
Protocols: HTTP
Category: http
---
If the server reports that the requested page has moved to a different
location (indicated with a Location: header and a 3XX response code), this
option will make carl redo the request on the new place. If used together with
--include or --head, headers from all requested pages will be shown. When
authentication is used, carl only sends its credentials to the initial
host. If a redirect takes carl to a different host, it won't be able to
intercept the user+password. See also --location-trusted on how to change
this. You can limit the amount of redirects to follow by using the
--max-redirs option.

When carl follows a redirect and if the request is a POST, it will do the
following request with a GET if the HTTP response was 301, 302, or 303. If the
response code was any other 3xx code, carl will re-send the following request
using the same unmodified method.

You can tell carl to not change POST requests to GET after a 30x response by
using the dedicated options for that: --post301, --post302 and --post303.

The method set with --request overrides the method carl would otherwise select
to use.
