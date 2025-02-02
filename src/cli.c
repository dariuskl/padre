// This is free and unencumbered software released into the public domain.

#include "padre.h"

// Provides access to all command-line arguments that were parsed.
typedef struct {
  account acc;
} cli_opts;

void print_usage(void) {
  println("usage: padre <domain> <username> [OPTIONS...]");
}

void print_help(void) {
  println("\n"
"Derives a deterministic password from <domain> and <username> and a master \n"
"password. Optionally a password iteration number may be given to generate  \n"
"new passwords for a combination of domain and username.                    \n"
"                                                                           \n"
"Instead of passing domain and username as arguments, a CSV entry can be    \n"
"piped into the standard input by giving the padre command only a dash.     \n"
"The entry must be structured as follows.                                   \n"
"    <domain>,<username>,<iteration>,<length>,<characters>                  \n"
"                                                                           \n"
"  -c, --chars=:graph:     List of characters or the name of a POSIX        \n"
"                          character class to use in the generated password \n"
"                          (regexp notation).                               \n"
"  -i, --iter=0            Password iteration number.                       \n"
"  -l, --length=64         Length of the generated password.                \n"
"  -h, --help              Give this help list and exit.                    \n"
"  -v, --version           Print version and exit.                          \n"
  );
}

void print_version(void) {
  println("padre v0.4");
}

utf8 next_arg(u8 ***pargv) {
  if (!**pargv)
    return (utf8){};
  utf8 arg = to_utf8(**pargv);
  *pargv = *pargv + 1;
  return arg;
}

void cli_help(void) {
  print_usage();
  print_help();
  exit_with_failure();
}

void cli_version(void) {
  print_version();
  exit_with_failure();
}

void cli_unknown_option(utf8 /*arg*/) {
  println("error: unknown option");
  print_usage();
  exit_with_failure();
}

void cli_set_option_s(utf8 *store, u8 ***pargv) {
  utf8 arg = next_arg(pargv);
  if (utf8_empty(arg)) {
    println("error: missing argument to option");
    exit_with_failure();
  }
  *store = arg;
}

void cli_set_option_i32(i32 *store, u8 ***pargv) {
  utf8 arg = next_arg(pargv);
  if (utf8_empty(arg)) {
    println("error: missing argument to option");
    exit_with_failure();
  }
  if (!scan_i32(&arg, store)) {
    println("error: invalid argument to option");
    exit_with_failure();
  }
}

#define cli_set_option(store, pargv) (                                        \
  _Generic((*store),                                                          \
    utf8: cli_set_option_s,                                                   \
    i32: cli_set_option_i32                                                   \
  )((store), (pargv)))

void cli_positional(cli_opts *options, utf8 arg) {
  if (!options->acc.domain.begin) {
    options->acc.domain = arg;
  } else if (!options->acc.username.begin) {
    options->acc.username = arg;
  } else {
    println("error: too many arguments");
    print_usage();
    exit_with_failure();
  }
}

cli_opts cli_parse(u8 *argv[], u8 */*envp*/[]) {
  cli_opts options = {};

  for (utf8 arg = next_arg(&argv); arg.begin; arg = next_arg(&argv)) {
    if (utf8_startswith(arg, utf8("--"))) { // long
      arg.begin += 2; // skip dashes
      if (utf8_eq(arg, utf8("help"))) {
        cli_help();
      } else if (utf8_eq(arg, utf8("version"))) {
        cli_version();
      } else if (utf8_eq(arg, utf8("chars"))) {
        cli_set_option(&options.acc.characters, &argv);
      } else if (utf8_eq(arg, utf8("iter"))) {
        cli_set_option(&options.acc.iteration, &argv);
      } else if (utf8_eq(arg, utf8("length"))) {
        cli_set_option(&options.acc.length, &argv);
      } else {
        cli_unknown_option(arg);
      }
    } else if (utf8_startswith(arg, utf8("-"))) { // short
      ++arg.begin; // skip dash
      if (utf8_empty(arg)) {
        cli_positional(&options, utf8("-"));
      } else for (u32 c; (c = utf8_nextch(&arg)); ) {
        switch (c) {
          case 'h':  cli_help();                                      break;
          case 'v':  cli_version();                                   break;
          case 'c':  cli_set_option(&options.acc.characters, &argv);  break;
          case 'i':  cli_set_option(&options.acc.iteration, &argv);   break;
          case 'l':  cli_set_option(&options.acc.length, &argv);      break;
          default:   cli_unknown_option(arg);                         break;
        }
      }
    } else {
      cli_positional(&options, arg);
    }
  }

  if (utf8_empty(options.acc.domain)
      || (!utf8_eq(options.acc.domain, utf8("-"))
          && utf8_empty(options.acc.username))) {
    println("error: not enough arguments");
    print_usage();
    exit_with_failure();
  }

  return options;
}
