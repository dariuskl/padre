// This is free and unencumbered software released into the public domain.

#ifndef PADRE_H_INCLUDED
#define PADRE_H_INCLUDED

#include "nonstd.h"

// The length of the input buffer is fixed to 64 bytes because I assume that
// anyone that can memorize a longer password does not need this utility.
#define MAX_MASTER_PASSWORD_LENGTH  64

// The length of what can be read from stdin. Will be allocated statically at
//  program startup.
#define MAX_INPUT_SIZE              1024

// These settings correspond with the defaults of the Python scrypt bindings.
// ... for historical reasons ...
#define MP_N 16384
#define MP_r 8
#define MP_p 1

typedef struct {
    utf8 domain;      // domain or - (dash) for stdin
    utf8 username;
    utf8 iteration;
    utf8 characters;  // the permissible characters for the password
    utf8 length;      // the length the generated password should have
} account;

#endif // PADRE_H_INCLUDED
