//
//   Copyright 2015 Darius Kellermann
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//

#include "padre.h"

#include <scrypt-kdf.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
derive_password(const size_t master_password_len,
                const char master_password[static master_password_len],
                const char *domain, const char *username, const char *passno,
                const size_t buf_len, char buf[static buf_len]) {
  const size_t salt_len = strlen(domain) + strlen(username) + strlen(passno);
  char *const salt = malloc(salt_len + 1);
  if (salt == nullptr) {
    perror("Could not allocate memory for the salt");
    return -1;
  }
  memcpy(salt, domain, strlen(domain) + 1);
  memcpy(salt + strlen(salt), username, strlen(username) + 1);
  memcpy(salt + strlen(salt), passno, strlen(passno) + 1);

  const int ret = crypto_scrypt((const uint8_t *)master_password,
                                master_password_len, (uint8_t *)salt, salt_len,
                                MP_N, MP_r, MP_p, (uint8_t *)buf, buf_len);

  free(salt);

  return ret;
}

// converts the bytes that scrypt spits out into characters from `chars`
static char *to_chars(uint8_t *bytes, size_t len, const char *chars,
                      const size_t clen) {
  for (; len > 0; --len) {
    char *str = (char *)bytes;
    *str = chars[*bytes % (int)clen];
    ++bytes;
  }
  *bytes = '\0';

  return (char*)bytes;
}

static char *push_char(char *begin, const char *end, const char c) {
  if (begin != nullptr && begin < end) {
    *begin = c;
    return begin + 1;
  }
  return nullptr;
}

static int enumerate_charset(const char *spec, char **res, size_t *rlen) {
  if (spec == nullptr || res == nullptr || rlen == nullptr) {
    errno = EINVAL;
    return -1;
  }

  // Resolve character classes.  If no `spec` is given, assume all ASCII
  // characters may be used.
  if (strcmp(spec, "") == 0 || strcmp(spec, ":graph:") == 0 ||
      strcmp(spec, "*") == 0) {
    spec = "!-~";
  } else if (strcmp(spec, ":alnum:") == 0) {
    spec = "a-zA-Z0-9";
  } else if (strcmp(spec, ":alpha:") == 0) {
    spec = "a-zA-Z";
  } else if (strcmp(spec, ":digit:") == 0) {
    spec = "0-9";
  } else if (strcmp(spec, ":lower:") == 0) {
    spec = "a-z";
  } else if (strcmp(spec, ":punct:") == 0) {
    spec = "!-/:-@[-`{-~";
  } else if (strcmp(spec, ":upper:") == 0) {
    spec = "A-Z";
  } else if (strcmp(spec, ":word:") == 0) {
    spec = "A-Za-z0-9_";
  } else if (strcmp(spec, ":xdigit:") == 0) {
    spec = "A-Fa-f0-9";
  } else if (strcmp(spec, "*") == 0) {
    spec = "!-~öäüµ€§°";
  }

  char chrs[95]; // as big as `|*|` plus one for the \0
  char l;        // left side of a character range
  int op;        // operator found (`-`) <- not a smiley!

  char *result = chrs;
  const char *end = chrs + 95;

  l = '\0';
  op = 0;
  while (*spec != '\0') {
    const char c = *spec;
    if (isspace(c))
      continue;

    if (l == '\0' && c == '-') {
      result = push_char(result, end, c);
    } else if (l == '\0' && c != '-') {
      l = c;
    } else if (l != '\0' && c == '-') {
      op = 1;
    } else if (l != '\0' && c != '-' && op == 1) {
      for (; l <= c; l++) {
        result = push_char(result, end, l);
      }
      op = 0;
      l = '\0';
    } else if (l != '\0' && c != '-' && op == 0) {
      result = push_char(result, end, l);
      l = c;
    }

    ++spec;
  }

  if (l != '\0') {
    result = push_char(result, end, l);
  }
  if (op == 1) {
    result = push_char(result, end, '-');
  }
  result = push_char(result, end, '\0');

  if (result == nullptr) {
    errno = EINVAL;
    return -1;
  }

  *rlen = strlen(chrs);
  *res = malloc(*rlen);
  if (*res == nullptr) {
    perror("While enumerating the charset");
    return -1;
  }
  memcpy(*res, chrs, *rlen + 1);

  return 0;
}

struct account {
  const char *domain;
  const char *username;
  const char *iteration;
  const char *characters; // the permissible characters for the password
  size_t length;          // the length the generated password should have
};

struct account_list {
  struct account *accounts;
  size_t size;
  size_t capacity;
};

static struct account_list parse_accounts(char *begin, const char *end) {
  struct account_list list = {
      .accounts = nullptr,
      .capacity = end - begin < AVERAGE_DATABASE_ENTRY_SIZE
                      ? 1
                      : (size_t)((end - begin) / AVERAGE_DATABASE_ENTRY_SIZE),
  };
  list.accounts = calloc(list.capacity, sizeof(struct account));

  const char *cur = begin;
  for (char *str = begin; str != end; ++str) {
    if (list.size == list.capacity) {
      list.capacity *= 2;
      list.accounts =
          realloc(list.accounts, list.capacity * sizeof(struct account));
    }

    switch (*str) {
    case '\n':
      *str = '\0';
      list.accounts[list.size].characters = cur;
      ++list.size;
      cur = str + 1;
      break;
    case ',':
      *str = '\0';
      if (list.accounts[list.size].domain == nullptr) {
        list.accounts[list.size].domain = cur;
      } else if (list.accounts[list.size].username == nullptr) {
        list.accounts[list.size].username = cur;
      } else if (list.accounts[list.size].iteration == nullptr) {
        list.accounts[list.size].iteration = cur;
      } else if (list.accounts[list.size].length == 0) {
        // TODO check length for validity
        list.accounts[list.size].length = (size_t)atoi(cur);
      } else {
        fprintf(stderr, "Error: too many columns in database file, line %zu",
                list.size + 1);
        free(list.accounts);
        list.accounts = nullptr;
        return list;
      }
      cur = str + 1;
      break;
    default:
      break;
    }
  }
  if (cur < end) { // missing a newline at the end of the file
    list.accounts[list.size].characters = cur;
    ++list.size;
  }
  return list;
}
