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

u8 input_buffer[MAX_INPUT_SIZE];

utf8 read_stdin(void) {
  buf8 buf = buf8(input_buffer);
  while (scan(&buf) > 0) {
  }
  return (utf8){buf.begin, buf.eod};
}

account determine_account(const cli_opts options) {
  if (utf8_eq(options.acc.domain, utf8("-"))
      && utf8_empty(options.acc.username)) {
    // read account from stdin
    utf8 buf = utf8_trim(read_stdin());

    if (utf8_empty(buf)) {
      println("error: nothing on stdin even though dash was given");
      exit_with_failure();
    }

    return csv_parse_account(buf);
  }
  // the account is specified on the command-line
  return options.acc;
}

i32 entry(i32 /*argc*/, u8 *argv[], u8 *envp[]) {
  cli_opts opts = cli_parse(argv, envp);
  account acc = determine_account(opts);

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
      || length <= 0 || length > MAX_INPUT_SIZE) {
    println("error: invalid arguments");
    exit_with_failure();
  }

  buf8 password = buf8(input_buffer);

  // ask the user for his master password    | no program exit between here ...
  u8 mp_buf[MAX_MASTER_PASSWORD_LENGTH];
  buf8 master_pwd = buf8(mp_buf);
  tui_ask_password(&master_pwd);

  int ret = derive_password((utf8){master_pwd.begin, master_pwd.eod},
                            acc.domain, acc.username, acc.iteration,
                            &password, length);

  clear_s(master_pwd.begin, master_pwd.eod);
  master_pwd.eod = master_pwd.begin;
  // clear the master password               | ... and here

  if (ret != 0) {
    println("error deriving the domain password");
    exit_with_failure();
  }

  utf8 chars = enumerate_charset(acc.characters);
  to_chars(password, chars);
  *password.eod = u8'\n';
  ++password.eod;
  print(((utf8){password.begin, password.eod}));

  return 0;
}

#include "nonstd.c"
