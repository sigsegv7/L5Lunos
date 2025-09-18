# L5Lunos - a clean, modern Solaris

Written mostly solo and completely from scratch, not Linux based nor does it
share any existing operating system sources.

## Current status

**Under heavy development**


## Progress

- 57-bit (5-level) paging
- 48-bit (4-level) paging
- Printf-style kernel logging
- Video console
- I/O APIC
- Local APIC timer(s)
- Processor local APIC(s)
- Syscall windows, domains and platform latches
- 16550 UART I/O
- i8254
- i8259

## Build instructions

To build the system, run the following to generate an ISO image:

```sh
tools/bootstrap.sh             # Build and fetch prerequisites
tools/tools/build-toolchain.sh # Build the cross compilation toolchain
make
```

## License

This project is licensed under the BSD 3 clause

SPDX identifier: BSD-3-Clause
