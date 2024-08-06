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

#include "padre.h"

#include <curses.h>
#include <menu.h>

#include <termios.h>

#include <stdlib.h>
#include <unistd.h>

struct tui_item {
  const char *name;
  char description[256];
};

static int tui__wait_user_selection(MENU *menu) {
  for (int c; (c = getch()) != KEY_F(1); refresh()) {
    switch (c) {
    case KEY_DOWN:
      menu_driver(menu, REQ_DOWN_ITEM);
      break;
    case KEY_UP:
      menu_driver(menu, REQ_UP_ITEM);
      break;
    case '\n':
      return item_index(current_item(menu));
    default:
      break;
    }
  }
  return -1;
}

static int tui_show_menu(const size_t num_items,
                         const struct tui_item items[static num_items]) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  ITEM **nc_items = malloc((num_items + 1) * sizeof(ITEM *));

  for (size_t i = 0; i < num_items; ++i) {
    nc_items[i] = new_item(items[i].name, items[i].description);
  }
  nc_items[num_items] = nullptr;

  MENU *menu = new_menu(nc_items);
  mvprintw(LINES - 2, 0, "press [F1] to exit, [ENTER] to select");
  post_menu(menu);
  refresh();

  const int selected_item = tui__wait_user_selection(menu);

  for (size_t i = 0; i < num_items; ++i) {
    free_item(nc_items[i]);
  }
  free_menu(menu);
  endwin();
  free(nc_items);

  return selected_item;
}

// TODO use ncurses here as well
// Asks the user for his master password, stores it in `passwd` and updates the
// length in `len`.
//   len — The length of the buffer resp. the length of the read password
//         string not including the terminating null byte.
// Returns 0 on success; -1 in case of a failure.
static int tui_ask_password(char *passwd, size_t *len) {
  static struct termios term_noecho, term_default;
  int c;

  /* turn off echoing */
  tcgetattr(STDIN_FILENO, &term_default);
  term_noecho = term_default;
  term_noecho.c_lflag &= (unsigned)~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term_noecho);

  fprintf(stdout, "Enter the master password: ");
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
