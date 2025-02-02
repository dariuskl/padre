// This is free and unencumbered software released into the public domain.

#ifndef PADRE_H_INCLUDED
#define PADRE_H_INCLUDED

// The length of the input buffer is fixed to 64 bytes because I assume that
// anyone that can memorize a longer password does not need this utility.
#define MAX_MASTER_PASSWORD_LENGTH  64

// The limit for the generated password length. Anything above 128 characters
// seems unreasonable.
#define MAX_PASSWORD_LENGTH 128

// These settings correspond with the defaults of the Python scrypt bindings.
// ... for historical reasons ...
// A good summary of what these numbers mean can be found here:
//   https://words.filippo.io/the-scrypt-parameters/
// In particular, you might want to consider doubling N nowadays.
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
