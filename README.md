# Padre — A Password Derivator

A simple password derivator with a focus on long-term stability and flexible
use.

Password derivators work by deriving complex passwords based on a single master
password. Given a domain, username, and an optional sequence number, `padre`
yields a password of configurable length and characters.

## Usage

To generate a password for a user `my_username` of the website `example.com`,
one would use below command.

    padre example.com my_username

Realizing that there are constraints by the website, one can configure the
password shall be 32 characters long and consist only of letters, numbers,
and `!` or `$` as below.

    padre example.com my_username -l 32 -c 'a-zA-Z0-9!$'

After getting notified by [haveibeenpwned] the password can be changed by
generating another iteration.

[haveibeenpwned]: https://haveibeenpwned.com

    padre example.com my_username -i 1 -l 32 -c 'a-zA-Z0-9!$'

The argument to iteration is used as a salt, so users are not limited to
numbers there.

### Keeping a database of accounts

In order to not have to type stuff like that over and over again, a CSV file
containing the accounts can be used in conjunction with `grep`.

    echo "example.com,my_username,1,32,a-zA-Z0-9!$" >> accounts.csv
    grep example.com accounts.csv | padre -

### Providing the password as a QR code

I often find myself generating passwords that I then need to transfer to my
mobile phone. This can be done by having a trustworthy QR code app on the phone
and generating a QR code from the password using the [`qrencode` utility].

    padre (...) | qrencode -t ansiutf8

[`qrencode` utility]: https://fukuchi.org/works/qrencode/

## Building

Padre can be built on pretty much any Linux system. It has zero dependencies
that need to be provided, not even a standard library or run-time.

Internally, `padre` uses [scrypt] v1.3.2 as its key-derivation function. To
get the standard library out of the picture, the source code of `scrypt` was
modified to some extent. This mostly means throwing out stuff, pasting the
remainder together into a single file, and changing a bunch of type aliases.

[scrypt]: https://www.tarsnap.com/scrypt.html

`scrypt` in turn uses SHA-256 and an implementation based on the one from the
[WJCryptLib] is provided. Similarly, it was copied here and adapted.

[WJCryptLib]: https://github.com/WaterJuice/WjCryptLib/

Build padre as follows.

    make

Tests are provided via shell script `test.sh`. To run the script you must have
the `expect` utility available.

## Implementation notes

The program is built in one step, following the "jumbo build" principle.

A lot of resources allocated throughout the code are not freed. This is on
purpose. It is much easier to just let the OS release the resources when the
process exits in such a short-lived program.

- `cli.c` — the command-line interface parser
- `tui.c` — the terminal UI for entering the master password
- `padre.c` — the password-derivation logic
- `main.c` — `entry()` point, program flow

The dependency graph is shown below. The top row consists of libraries while
other rows contain files.

                                               ┌──────────┐
                                               │ sha256.c │
                                               └──────────┘
                                                    ↑
                                               ┌──────────┐
                                               │ scrypt.c │
                                               └──────────┘
                                                    ↑
    ┌───────┐            ┌───────┐              ┌─────────┐
    │ cli.c │            │ tui.c │              │ padre.c │
    └───────┘            └───────┘              └─────────┘
        ↑                    ↑                       ↑
        └────────────────────┼───────────────────────┘
                        ┌────────┐
                        │ main.c │
                        └────────┘
                             ↑
                        ┌─────────┐
                        │ linux.c │
                        └─────────┘

## UNLICENSE

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org/>
