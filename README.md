# Padre — Password Derivator

A simple password derivator with a focus on long-term stability and ease-of-use
featuring a command-line and graphical user interface.

It works by deriving complex passwords based on a master password for any
account given by domain, username, and an optional sequence number.

## Building

Padre can be built on any Linux system that can build its dependencies (which
should be pretty much any).

Download the latest release tarball for scrypt from
[the official scrypt webpage] and verify its integrity according to the
instructions given on that page.
Then build and install scrypt as follows.

    ./configure --enable-libscrypt-kdf
    make install

[the official scrypt webpage]: https://www.tarsnap.com/scrypt.html

The GUI requires SDL2 to be present in the system. It is usally best obtained
via the system package manager.

Finally, build padre as follows.

    make

## LICENSE

Licensed under the Apache License, Version 2.0, see LICENSE.
