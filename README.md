Padre (CLI)
===========

A simple and secure password derivator with an easy-to-use command-line
interface implemented in C.

There is a compatible GUI as well:

        https://github.com/dariuskl/padre


TODO
----

  - Support for Unicode
  - Implement a DBus interface to let other applications (i.e. web browsers)
    query or add passwords for domains. It might be sensible to split padre into
    two modules: the backend application and the user interface. The latter
    would communicate with the backend via DBus just like any other application.
  - Implement an Epiphany plugin to demonstrate usage.
  - Internationalization


LICENSE
-------

Licensed under the Apache License, Version 2.0, see LICENSE.
