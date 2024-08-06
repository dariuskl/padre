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

#include "padre.c"

#include <argp.h>
#include <termios.h>

#include <stdint.h>
#include <string.h>

// Asks the user for his master password, stores it in `passwd` and updates the
// length in `len`.
//   len — The length of the buffer resp. the length of the read password
//         string not including the terminating null byte.
// Returns 0 on success; -1 in case of a failure.
static int ask_password(char *passwd, size_t *len) {
  static struct termios term_noecho, term_default;
  int c;

  /* turn off echoing */
  tcgetattr(STDIN_FILENO, &term_default);
  term_noecho = term_default;
  term_noecho.c_lflag &= (unsigned)~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term_noecho);

  fprintf(stdout, "Enter your master password: ");
  fflush(stdout);
  size_t curr_len = 0;
  while ((c = fgetc(stdin)) != EOF && c != '\n' && curr_len < *len) {
    passwd[curr_len] = (char)c;
    curr_len++;
  }
  passwd[curr_len] = '\0';

  *len = curr_len;

  /* turn echoing back on */
  tcsetattr(STDIN_FILENO, TCSANOW, &term_default);
  fprintf(stdout, "\n");

  if (c == EOF)
    return -1;
  return 0;
}

// Provides access to all command-line arguments that were parsed.
struct cli_opts {
  const char *domain;
  const char *username;
  const char *passno;
  const char *chars;
  size_t dkLen;
} options = {nullptr, nullptr, nullptr, nullptr, 0};

static error_t parse_opt(const int key, char *arg, struct argp_state *state) {
  int tmp;

  switch (key) {
  case 'c':
    options.chars = arg;
    break;
  case 'l':
    tmp = atoi(arg);
    if (tmp == 0 || tmp < 0) {
      fprintf(stderr, "The length of the derived password can't be"
                      " negative or zero.\n");
      return EINVAL;
    }
    options.dkLen = (size_t)tmp;
    break;
  case 'n':
    options.passno = arg;
    break;

  case ARGP_KEY_ARG:
    switch (state->arg_num) {
    case 0:
      options.domain = arg;
      break;
    case 1:
      options.username = arg;
      break;
    default:
      fputs("error: too many arguments\n", stderr);
      argp_usage(state);
      return EINVAL;
    }
    break;

  case ARGP_KEY_END:
    if (state->arg_num < 1 || state->arg_num > 2) {
      fputs("error: wrong number of arguments\n", stderr);
      argp_usage(state);
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp_option argp_options[] = {
    {"length", 'l', "64", 0, "Length of the generated password.", 0},
    {"passno", 'n', "0", 0, "Password iteration number.", 0},
    {"chars", 'c', ":graph:", 0,
     "List of characters or the name of a POSIX character class to use in"
     " the generated password (regexp notation).",
     0},
    {nullptr}};

static struct argp argp = {
    argp_options,
    &parse_opt,
    "<domain> <username>\n<database>",
    "Generates a deterministic password from the <domain> and <username>."
    " Optionally a password iteration number may be given to generate"
    " a new password for the same combination of domain and username.",
    nullptr,
    nullptr,
    nullptr};

int main(const int argc, char *argv[]) {
  int ret = argp_parse(&argp, argc, argv, 0, 0, &options);
  if (ret != 0)
    exit(1);

  if (options.domain == nullptr) {
    // start ncurses UI
    initscr();
    atexit(quit);
    clear();
    noecho();
    curs_set(0);
    cbreak();
    nl();
    keypad(stdscr, TRUE);
    for (;; refresh()) {
    }
    endwin();
  }

  if (options.passno == nullptr) {
    options.passno = "0";
  }

  if (options.dkLen == 0) {
    options.dkLen = 64;
  }

  if (options.chars == nullptr) {
    options.chars = "";
  }

  uint8_t *buf = malloc(options.dkLen + 1);
  if (buf == NULL) {
    perror("Could not allocate memory for the domain password");
    exit(1);
  }

  // Ask the user for his master password. The length of the input buffer
  // is fixed to 64 bytes because no human being that needs a password
  // derivator can memorize a password longer than 63 characters anyway.
  char *const master_pwd = malloc(64);
  if (master_pwd == nullptr) {
    perror("Could not allocate memory for the master password");
    exit(1);
  }
  size_t master_pwd_len = 64;
  ret = ask_password(master_pwd, &master_pwd_len);
  if (ret != 0) {
    perror("While reading the master password from `stdin`");
    goto fail;
  }

  ret = derive_password(master_pwd_len, master_pwd, options.domain,
                        options.username, options.passno, options.dkLen, buf);

  memset(master_pwd, 0, 64);
  master_pwd_len = 0;
  if (ret != 0) {
    perror("While deriving the domain password");
    goto fail;
  }

  char *chars;
  size_t len;
  ret = enumerate_charset(options.chars, &chars, &len);
  if (ret != 0) {
    perror("While enumerating the charset");
    goto fail;
  }
  to_pwdchars((char *)buf, options.dkLen, chars, len);
  fprintf(stdout, "%s\n", (const char *)buf);

  exit(0);

fail:
  memset(master_pwd, 0, 64);
  master_pwd_len = 0;
  exit(1);
}
