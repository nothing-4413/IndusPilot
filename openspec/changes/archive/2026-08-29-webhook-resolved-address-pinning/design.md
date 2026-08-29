# Design: Validated address connector

The webhook resolver returns the first address only after every `getaddrinfo` result has passed the existing public-address policy. The selected address is rendered with `inet_ntop` and passed to Drogon's IP/port client constructor. The request carries the original authority in its `Host` header, including a non-default port, so HTTP virtual hosting remains correct.

TLS certificate verification remains enabled. The IP overload does not expose an independent SNI hostname in the installed Drogon API, so the implementation does not disable verification or pretend that hostname SNI is preserved. HTTPS services requiring DNS-name SNI must be integrated through a future connector with explicit SNI support.
