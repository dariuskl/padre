# Padre — A Password Derivator

A simple password derivator with a focus on long-term stability and ease-of-use
featuring a command-line and graphical user interface.

It works by deriving complex passwords based on a master password for any
account given by domain, username, and an optional sequence number.

## Usage

To generate a password for a user `my_username` of the website `domain.com`,
one would use below command.

    padre domain.com my_username

Realizing that there are constraints by the website, one can configure the
password shall be 32 characters long and consist only of letters, numbers,
and `!` or `$` as below.

    padre domain.com my_username -l 32 -c 'a-zA-Z0-9!$'

After getting notified by [haveibeenpwned] the password can be changed by
generating another iteration.

[haveibeenpwned]: https://haveibeenpwned.com

    padre domain.com my_username -i 1 -l 32 -c 'a-zA-Z0-9!$'

In order to not have to type stuff like that over and over again, a CSV file
containing the accounts can be used.

    echo "domain.com,my_username,1,32,a-zA-Z0-9!$" >> accounts.csv
    padre accounts.csv

### Providing the password as a QR code

I often find myself generating passwords that I then need to transfer to my
mobile phone. This can be done by having a trustworthy QR code app on the phone
and generating a QR code from the password using the [`qrencode` utility].

    padre > qrencode -t ansiutf8

[`qrencode` utility]: https://fukuchi.org/works/qrencode/

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

The GUI requires ncurses to be present in the system. It is usually best
obtained via the system package manager.

Finally, build padre as follows.

    make

## Implementation notes

The program is built in one step, following the "jumbo build" principle.

A lot of resources allocated throughout the code are not freed. This is on
purpose. It is much easier to just let the OS release the resources when the
process exits in such a short-lived program.

- `cli.c` — the command-line interface parser
- `tui.c` — the terminal UI for selecting account and entering master password
- `padre.c` — the password-derivation logic
- `main.c` — `main()`, file management, program flow

The dependency graph is shown below. The top row consists of libraries while
other rows contain files.

    ┌──────┐            ┌─────────┐             ┌────────┐
    │ argp │            │ ncurses │             │ scrypt │
    └──────┘            └─────────┘             └────────┘
       ↑                     ↑                      ↑
    ┌───────┐            ┌───────┐              ┌─────────┐
    │ cli.c │            │ tui.c │              │ padre.c │
    └───────┘            └───────┘              └─────────┘
        ↑                    ↑                       ↑
        └────────────────────┼───────────────────────┘
                        ┌────────┐
                        │ main.c │
                        └────────┘

## LICENSE

Licensed under the Apache License, Version 2.0, see LICENSE.
