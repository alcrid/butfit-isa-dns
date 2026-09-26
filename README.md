# isa-dns

A DNS filter written in C++. It blocks type A queries for domains on a
blocklist (and all their subdomains) and forwards everything else unchanged to
an upstream resolver, then passes the resolver's reply back to the client.

School project for **Network Applications and Network Administration (ISA)**
at FIT BUT. DNS messages are parsed and built by hand, only sockets, pthreads
and the C/C++ standard library are used. Assignment (in Czech):
[`doc/assignment.md`](doc/assignment.md).

## Features

- blocks A queries for listed domains **and their subdomains**
  (`youtube.com` in the list also blocks `music.youtube.com`)
- forwards allowed queries to any resolver (IP address or hostname) and relays the
  reply back byte for byte
- UDP, concurrent queries handled in separate threads (pthread)
- answers with the right DNS error code instead of dropping the packet:

  | situation | RCODE |
  |---|---|
  | domain is on the blocklist | `5 REFUSED` |
  | query type other than A (AAAA, MX, ...) | `4 NOT IMPLEMENTED` |
  | malformed packet | `1 FORMAT ERROR` |
  | resolver doesn't answer | `2 SERVER FAILURE` |

- blocklist accepts Linux and Windows line endings, trims whitespace, skips empty
  lines and `#` comments, matching is case-insensitive
- `-v` verbose logging, clean shutdown on `SIGINT`/`SIGTERM`, no memory leaks
  under valgrind

## Build

```bash
make          # builds ./dns
make clean
```

Needs `g++` with C++17 on Linux.

## Usage

```
./dns -s server [-p port] -f filter_file [-v] [-h]
```

| option | meaning |
|---|---|
| `-s server` | upstream resolver, IP address or hostname (e.g. `8.8.8.8`, `dns.google`) |
| `-p port` | port to listen on, default `53` (needs root) |
| `-f filter_file` | blocklist, one domain per line |
| `-v` | print what is happening to each query |
| `-h` | help |

Arguments can be in any order.

## Examples

Blocklist `blocked.txt`:

```
# social
youtube.com
instagram.com

alza.sk
```

Start the filter on an unprivileged port and use Google as the resolver:

```bash
./dns -s 8.8.8.8 -p 5353 -f blocked.txt -v
```

Query it from another terminal:

```bash
$ dig @127.0.0.1 -p 5353 vut.cz A          # allowed -> status: NOERROR + A record from 8.8.8.8
$ dig @127.0.0.1 -p 5353 youtube.com A     # blocked -> status: REFUSED
$ dig @127.0.0.1 -p 5353 music.youtube.com A   # subdomain of a blocked domain -> status: REFUSED
$ dig @127.0.0.1 -p 5353 google.com AAAA   # not an A query -> status: NOTIMP
```

Use it as the system resolver (port 53 needs root):

```bash
sudo ./dns -s 1.1.1.1 -f tests/blocklists/blocked_domains
```

`tests/blocklists/blocked_domains` is a ready-made ad server list of ~3400 domains
from [pgl.yoyo.org](https://pgl.yoyo.org/adservers/).

## Tests

```bash
make test          # both suites
make test-args     # command line parsing (17 tests)
make test-filter   # blocking, subdomains, whitespace, error codes, concurrency
```

The filter tests query the running filter with `dig` (package `bind` / `dnsutils`)
and `nc`, and need internet access to reach `dns.google`.

## Project structure

```
src/
  dns.cpp            argument parsing, signal handling, main
  dns_filter.cpp/hpp UDP server loop, threads, forwarding to the resolver
  dns_message.cpp/hpp DNS packet parsing and building, error responses
  blocklist.cpp/hpp  loading the list and domain/subdomain matching
  logger.cpp/hpp     verbose output
tests/
  argument_tests.sh, filter_tests.sh, blocklists/
doc/
  assignment.md
```
