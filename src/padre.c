// This is free and unencumbered software released into the public domain.

#include "padre.h"
#include "nonstd.h"

#include "scrypt.c"

int derive_password(arena *a, utf8 master_password, utf8 domain, utf8 username,
                    utf8 passno, buf8 *password) {
  size salt_len = utf8_len(domain) + utf8_len(username) + utf8_len(passno);

  buf8 salt = arena_push(a, salt_len);
  salt.eod = copy_b(domain.begin, domain.end, salt.eod, salt.end);
  salt.eod = copy_b(username.begin, username.end, salt.eod, salt.end);
  salt.eod = copy_b(passno.begin, passno.end, salt.eod, salt.end);

  return scrypt(a, master_password.begin, utf8_len(master_password),
                salt.begin, salt_len, MP_N, MP_r, MP_p, password);
}

// converts the bytes that the password derivator spits out
// into characters from `charset`
utf8 to_chars(buf8 bytes, utf8 charset) {
  for (; bytes.begin != bytes.eod; ++bytes.begin) {
    *bytes.begin = charset.begin[*bytes.begin % utf8_len(charset)];
  }
  return (utf8){bytes.begin, bytes.eod};
}

#define MAX_CHARSET_LENGTH 94  // as big as |*|

utf8 enumerate_charset(utf8 spec) {
  // Resolve character classes.  If no `spec` is given, assume all ASCII
  // characters may be used.
  if (utf8_empty(spec) || utf8_eq(spec, utf8(":graph:"))
      || utf8_eq(spec, utf8("*"))) {
    spec = utf8("!-~");
  } else if (utf8_eq(spec, utf8(":alnum:"))) {
    spec = utf8("a-zA-Z0-9");
  } else if (utf8_eq(spec, utf8(":alpha:"))) {
    spec = utf8("a-zA-Z");
  } else if (utf8_eq(spec, utf8(":digit:"))) {
    spec = utf8("0-9");
  } else if (utf8_eq(spec, utf8(":lower:"))) {
    spec = utf8("a-z");
  } else if (utf8_eq(spec, utf8(":punct:"))) {
    spec = utf8("!-/:-@[-`{-~");
  } else if (utf8_eq(spec, utf8(":upper:"))) {
    spec = utf8("A-Z");
  } else if (utf8_eq(spec, utf8(":word:"))) {
    spec = utf8("A-Za-z0-9_");
  } else if (utf8_eq(spec, utf8(":xdigit:"))) {
    spec = utf8("A-Fa-f0-9");
  } else if (utf8_eq(spec, utf8("*"))) {
    spec = utf8("!-~öäüµ€§°");
  }

  static u8 chars[MAX_CHARSET_LENGTH];
  buf8 result = buf8(chars);

  u32 l = '\0';           // left side of a character range
  bool is_range = false;  // operator found (`-`) <- not a smiley!
  u32 c;
  while ((c = utf8_nextch(&spec))) {
    if (utf8_is_ascii_space(c))
      continue;

    if (l == '\0' && c == '-') {
      buf8_push(&result, c);
    } else if (l == '\0' && c != '-') {
      l = c;
    } else if (l != '\0' && c == '-') {
      is_range = true;
    } else if (l != '\0' && c != '-' && is_range) {
      for (; l <= c; ++l) {
        buf8_push(&result, l);
      }
      is_range = false;
      l = '\0';
    } else if (l != '\0' && c != '-' && !is_range) {
      buf8_push(&result, l);
      l = c;
    }
  }

  if (l != '\0') {
    buf8_push(&result, l);
  }
  if (is_range) {
    buf8_push(&result, U'-');
  }

  return (utf8){result.begin, result.eod};
}
