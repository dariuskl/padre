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

#include <argp.h>

#include <stdlib.h>

// Provides access to all command-line arguments that were parsed.
struct cli_opts {
  const char *domain_or_database;
  const char *username;
  const char *iteration;
  const char *characters;
  size_t length;
};

static error_t parse_opt(const int key, char *arg, struct argp_state *state) {
  int tmp;

  struct cli_opts *options = state->input;

  switch (key) {
  case 'c':
    options->characters = arg;
    break;
  case 'l':
    tmp = atoi(arg);
    if (tmp == 0 || tmp < 0) {
      fputs("Error: the length of the derived password may not be negative or"
            " zero\n",
            stderr);
      return EINVAL;
    }
    options->length = (unsigned)tmp;
    break;
  case 'i':
    options->iteration = arg;
    break;

  case ARGP_KEY_ARG:
    switch (state->arg_num) {
    case 0:
      options->domain_or_database = arg;
      break;
    case 1:
      options->username = arg;
      break;
    default:
      fputs("Error: too many arguments\n", stderr);
      argp_usage(state); // exits
    }
    break;

  case ARGP_KEY_END:
    if (state->arg_num < 1) {
      fputs("Error: missing required argument(s)\n", stderr);
      argp_usage(state); // exits
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp_option cli_options[] = {
    {"length", 'l', "64", 0, "Length of the generated password.", 0},
    {"iter", 'i', "0", 0, "Password iteration number.", 0},
    {"chars", 'c', ":graph:", 0,
     "List of characters or the name of a POSIX character class to use in"
     " the generated password (regexp notation).",
     0},
    {nullptr}};

static struct argp cli_parser = {
    cli_options,
    &parse_opt,
    "<domain> <username>\n<database>",
    "Derives a deterministic password from <domain> and <username> and a"
    " master password. Optionally a password iteration number may be given to"
    " generate new passwords for a combination of domain and username.\n"
    "\n"
    "Instead of giving domain and username, the path to a CSV file can be"
    " given as first argument. If a dash is given, the file is read from"
    " the standard input. The file must be structured as follows.\n"
    "    <domain>,<username>,<iteration>,<length>,<characters>",
    nullptr,
    nullptr,
    nullptr};

static struct cli_opts cli_parse(const int argc, char *argv[]) {
  struct cli_opts options = {nullptr, nullptr, nullptr, nullptr, 0};

  argp_parse(&cli_parser, argc, argv, 0, 0, &options);

  return options;
}
