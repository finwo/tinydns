## Pending

### https://github.com/finwo/tinydns/pull/21

- Adds support for parallel requests instead of handling them sequentially

### https://github.com/finwo/tinydns/pull/20

- Fixed all compiler warnings to more clearly see regressions

### https://github.com/finwo/tinydns/commit/df615c5fe123e5c4ada35f5652132a990d78aba0

- Using [dep package manager][gh-dep] package manager instead of manually imported dependencies

### https://github.com/finwo/tinydns/pull/17

- Re-allow comments in json configuration since replacing config parser

### https://github.com/finwo/tinydns/pull/16

- Adds the ability to specify the configuration file to use using command-line arguments

### https://github.com/finwo/tinydns/pull/14

- Adds headerguards on all header files in the project to prevent double inclusion errors

### https://github.com/finwo/tinydns/pull/10

- Adds alternative port support to bound address in config
- Adds alternative port support to upstream server reference in config

### https://github.com/finwo/tinydns/pull/8

- Parse config using compliant json library instead of custom solution
    **CAUTION**: breaking change

### https://github.com/finwo/tinydns/pull/7

- Replaces custom argument parsing by cofyc/argparse library

### https://github.com/finwo/tinydns/pull/6

- Directory structure change
- Change whitespace in code from tabs to spaces
- Clearer definition of default configuration in config.c
- Added editorconfig to make contributions more consistent

## Fork 0.3.1

At this point, the project was forked from https://github.com/CupIvan/tinydns at
https://github.com/CupIvan/tinydns/commit/4e633cf205f8b0d6a4c6de3da1edeca0b31543d2
a.k.a. version 0.3.1

[gh-dep]: https://github.com/finwo/dep
