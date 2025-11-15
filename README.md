This project implements a dns filter that blocks the DNS type A requests for domains given
a predefined blocklist it also blocks the request if it is a subdomain of them. The reply from
the resolver is then redirected to the client without any changes done to the packet,
The implementation uses UDP protocol for all DNS communication and supports con-
current query handling through POSIX threads (pthread). Queries for non-A record types,
blocked domains, or malformed packets receive appropriate DNS error responses (RCODE).

Implemented features 