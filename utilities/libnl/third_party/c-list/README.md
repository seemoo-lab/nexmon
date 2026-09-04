c-list
======

Circular Intrusive Double Linked List Collection

The c-list project implements an intrusive collection based on circular double
linked lists in ISO-C11. It aims for minimal API constraints, leaving maximum
control over the data-structures to the API consumer.

### Project

 * **Website**: <https://c-util.github.io/c-list>
 * **Bug Tracker**: <https://github.com/c-util/c-list/issues>

### Requirements

The requirements for this project are:

 * `libc` (e.g., `glibc >= 2.16`)

At build-time, the following software is required:

 * `meson >= 0.60`
 * `pkg-config >= 0.29`

### Build

The meson build-system is used for this project. Contact upstream
documentation for detailed help. In most situations the following
commands are sufficient to build and install from source:

```sh
mkdir build
cd build
meson setup ..
ninja
meson test
ninja install
```

No custom configuration options are available.

### Repository:

 - **web**:   <https://github.com/c-util/c-list>
 - **https**: `https://github.com/c-util/c-list.git`
 - **ssh**:   `git@github.com:c-util/c-list.git`

### License:

 - **Apache-2.0** OR **LGPL-2.1-or-later**
 - See AUTHORS file for details.
