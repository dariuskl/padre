# Padre — Password Derivator

A simple password derivator with a focus on long-term stability and ease-of-use
featuring a command-line and graphical user interface.

It works by deriving complex passwords based on a master password for any
account given by

## Building

Download the latest release tarball for scrypt from
[the official scrypt webpage] and verify its integrity according to the
instructions given on that page.
Then build and install scrypt as follows.

    ./configure --enable-libscrypt-kdf
    make install

[the official scrypt webpage]: https://www.tarsnap.com/scrypt.html

## LICENSE

Licensed under the Apache License, Version 2.0, see LICENSE.
