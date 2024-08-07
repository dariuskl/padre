//
//   Copyright 2024 Darius Kellermann
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

#include "cli.c"
#include "padre.c"
#include "tui.c"

struct buffer {
  char *data;
  size_t size;
  size_t capacity;
};

void free_buffer(struct buffer *buffer) {
  free(buffer->data);
  *buffer = (struct buffer){nullptr, 0, 0};
}

// The buffer allocating in this function is never freed. This is on purpose.
// The file opened in this function is never closed. This is also on purpose.
// Resources are going to be released eventually when the program exits.
static struct buffer read_entire_file(const char *path) {
  struct buffer buf = {.data = nullptr, .capacity = 0};

  FILE *f = strcmp(path, "-") == 0 ? stdin : fopen(path, "r");
  if (!f) {
    perror(path);
    return buf;
  }

  buf.data = malloc(1024);
  buf.capacity = 1024;

  for (size_t bytes_read; (bytes_read = fread(buf.data, 1, 1024, f)) > 0;
       buf.size += bytes_read) {
    if (buf.size > buf.capacity) {
      buf.capacity *= 2;
      if (buf.capacity > MAX_DATABASE_FILE_SIZE) {
        fputs("Error: database file exceeds size limit\n", stderr);
        free(buf.data);
        buf.data = nullptr;
        return buf;
      }
      buf.data = realloc(buf.data, buf.capacity);
    }
  }

  return buf;
}

static struct account determine_account(const struct cli_opts options) {
  struct account account = {nullptr, nullptr, nullptr, nullptr, 0};

  if (options.username == nullptr) {
    // a database is specified on the command-line

    const struct buffer buf = read_entire_file(options.domain_or_database);
    if (buf.data == nullptr) {
      return account;
    }

    const struct account_list accounts =
        parse_accounts(buf.data, buf.data + buf.size);

    if (accounts.size == 0) {
      fputs("Error: database file contains no entries\n", stderr);
      return account;
    }

    if (accounts.size == 1) {
      account = accounts.accounts[0];
      fputs("Warning: automatically selected the only available account\n",
            stderr);
      return account;
    }

    struct tui_item *items = malloc(accounts.size * sizeof(struct tui_item));
    for (size_t i = 0; i < accounts.size; ++i) {
      items[i].name = accounts.accounts[i].domain;
      snprintf(items[i].description, sizeof items[i].description,
               "%s, iteration %s", accounts.accounts[i].username,
               accounts.accounts[i].iteration);
    }

    const int selected_account = tui_show_menu(accounts.size, items);
    if (selected_account < 0) {
      perror("Error trying to show ncurses menu");
    } else {
      account = accounts.accounts[selected_account];
    }

    free(accounts.accounts);
    free(items);

  } else {
    // the account is specified on the command-line

    account = (struct account){
        .domain = options.domain_or_database,
        .username = options.username,
        .iteration = options.iteration ? options.iteration : "0",
        .characters = options.characters ? options.characters : "",
        .length = options.length ? options.length : 64};
  }

  return account;
}

int main(const int argc, char *argv[]) {
  const struct account account = determine_account(cli_parse(argc, argv));

  if (!account.domain) {
    return EXIT_FAILURE;
  }

  char *password = malloc(account.length + 1);
  if (password == NULL) {
    perror("Error allocating memory for the derived password");
    return EXIT_FAILURE;
  }

  // ask the user for his master password    | no program exit between here ...
  char master_pwd[MAX_MASTER_PASSWORD_LENGTH];

  size_t master_pwd_len = MAX_MASTER_PASSWORD_LENGTH;
  int ret = tui_ask_password(master_pwd, &master_pwd_len);
  if (ret != 0) {
    perror("Error reading the master password from the standard input");
    return EXIT_FAILURE;
  }

  ret = derive_password(master_pwd_len, master_pwd, account.domain,
                        account.username, account.iteration, account.length,
                        password);

  // clear the master password               | ... and here
  memset(master_pwd, 0, MAX_MASTER_PASSWORD_LENGTH); // TODO explicit
  master_pwd_len = 0;

  if (ret != 0) {
    perror("Error deriving the domain password");
    return EXIT_FAILURE;
  }

  char *chars;
  size_t len;
  ret = enumerate_charset(account.characters, &chars, &len);
  if (ret != 0) {
    perror("Error enumerating the charset");
    return EXIT_FAILURE;
  }
  to_chars((uint8_t *)password, account.length, chars, len);
  fprintf(stdout, "%s\n", password);

  return EXIT_SUCCESS;
}
