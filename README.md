# TinyDNS

This is a tiny DNS server with simple JSON config written in C.

## Dependencies

This project relies on the [dep package manager][gh-dep] to manage it's dependencies

## Features

* filesize is just only 20Kb!
* cache DNS queries for boost internet connection
* resolve own domains from config
* resolve multidomains like *.example.com
* server can work on IPv6 address

## Compile and Install

* if you use [Archlinux](https://archlinux.org), you may install [tinydns from AUR](https://aur.archlinux.org/packages/tinydns/)
* for compile just run `make`
* after install you need to write your IP address in `/etc/tinydns.conf`
* you may also use `systemctl` for start and stop service

## Attributions

This project makes use of the following libraries, copied into the source
directory:

- [cofyc/argparse](https://github.com/cofyc/argparse)
- [kgabis/parson](https://github.com/kgabis/parson)

[gh-dep]: https://github.com/finwo/dep
