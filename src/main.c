// This is free and unencumbered software released into the public domain.

#include "cli.c"
#include "padre.c"
#include "tui.c"

#include "padre.h"
#include "nonstd.h"

void csv_push_field(account *acc, utf8 field) {
  if (utf8_empty(acc->domain)) {
    acc->domain = field;
  } else if (utf8_empty(acc->username)) {
    acc->username = field;
  } else if (utf8_empty(acc->iteration)) {
    acc->iteration = field;
  } else if (utf8_empty(acc->length)) {
    acc->length = field;
  } else if (utf8_empty(acc->characters)) {
    acc->characters = field;
  } else {
    // keep appending to the characters field
    acc->characters = (utf8){acc->characters.begin, field.end};
  }
}

account csv_parse_account(utf8 str) {
  const u8 *begin = str.begin;
  account acc = {};

  for (u32 c = utf8_nextch(&str); c; c = utf8_nextch(&str)) {
    if (c == ',') {
      csv_push_field(&acc, (utf8){begin, str.begin - 1});
      begin = str.begin;
    }
  }

  csv_push_field(&acc, (utf8){begin, str.begin});

  return acc;
}

utf8 read_stdin(buf8 *buf) {
  utf8 str = {buf->eod, buf->eod};
  while (scan(buf) > 0) {
  }
  str.end = buf->eod;
  return str;
}

account determine_account(buf8 *buf, const cli_opts options) {
  if (utf8_eq(options.acc.domain, utf8("-"))
      && utf8_empty(options.acc.username)) {
    // read account from stdin
    utf8 csv = utf8_trim(read_stdin(buf));

    if (utf8_empty(csv)) {
      println("error: nothing on stdin even though dash was given");
      exit_with_failure();
    }

    return csv_parse_account(csv);
  }
  // the account is specified on the command-line
  return options.acc;
}

// the backing for our arena, 1024 bytes for us, rest for scrypt
static u8 static_backing[1024 + MP_N * 2 * MP_r * 64 + 32 * 1024];
//                              ^~~~~~~~~~~~~~~~~~~~   ^~~~~~~~~
//                               scrypt must-have       overhead

i32 entry(i32 /*argc*/, u8 *argv[], u8 *envp[]) {
  // the central arena where all data is stored that is not on the stack
  arena a = {buf8(static_backing)};

  cli_opts opts = cli_parse(argv, envp);
  account acc = determine_account(&a.buf, opts);

  if (utf8_empty(acc.iteration)) {
    acc.iteration = utf8("0");
  }
  if (utf8_empty(acc.length)) {
    acc.length = utf8("64");
  }
  if (utf8_empty(acc.characters)) {
    acc.characters = utf8(":graph:");
  }

  i32 length;
  if (!scan_i32(&acc.length, &length)) {
    println("error: length not given as a decimal integer");
    exit_with_failure();
  }

  if (utf8_empty(acc.domain) || utf8_empty(acc.username)
      || utf8_empty(acc.iteration) || utf8_empty(acc.characters)
      || length <= 0 || length > MAX_PASSWORD_LENGTH) {
    println("error: invalid arguments");
    exit_with_failure();
  }

  // allocate a buffer for the generated password
  // one char extra for the line terminator
  buf8 password = arena_push(&a, length + 1);

  // ask the user for his master password    | no program exit between here ...
  buf8 master_pwd = arena_push(&a, MAX_MASTER_PASSWORD_LENGTH);
  tui_ask_password(&master_pwd);

  int ret = derive_password(&a, (utf8){master_pwd.begin, master_pwd.eod},
                            acc.domain, acc.username, acc.iteration,
                            &password);
  password.eod = password.begin + length;

  // clear the master password asap
  clear_s(master_pwd.begin, master_pwd.eod);
  master_pwd.eod = master_pwd.begin;

  if (ret != 0) {
    println("error deriving the domain password");
    ret = 1;
  } else {
    utf8 chars = enumerate_charset(acc.characters);
    to_chars(password, chars);
    *password.eod = u8'\n';
    ++password.eod;
    print(((utf8){password.begin, password.eod}));
  }

  // clear the whole arena
  clear_s(a.buf.begin, a.buf.end);

  //                                         | ... and here

  return ret;
}

#include "nonstd.c"
