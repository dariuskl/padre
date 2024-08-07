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

#include <stdlib.h>
#include <unistd.h>

struct tui_item {
  const char *name;
  char description[256];
};

static int tui__wait_user_selection(MENU *menu) {
  for (int c; (c = getch()) != 'q'; refresh()) {
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
  cbreak(); // get characters immediately, don't cache until line break
  noecho();
  keypad(stdscr, TRUE);

  ITEM **nc_items = malloc((num_items + 1) * sizeof(ITEM *));

  for (size_t i = 0; i < num_items; ++i) {
    nc_items[i] = new_item(items[i].name, items[i].description);
  }
  nc_items[num_items] = nullptr;

  attron(A_REVERSE);
  mvprintw(
      LINES - 2, 0,
      "Press [q] to quit or [ENTER] to select. Showing %d out of %zu items.",
      LINES - 2, num_items);
  attroff(A_REVERSE);
  mvprintw(LINES - 1, 0, "Type to search: not yet implemented :-(");

  MENU *menu = new_menu(nc_items);
  set_menu_format(menu, LINES - 3, 1);
  const int ret = post_menu(menu);
  if (ret != E_OK) {
    fprintf(stderr, "Error trying to show ncurses menu: %d\n", ret);
    endwin();
    return -1;
  }
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

// Asks the user for his master password, stores it in `passwd` and updates the
// length in `len`.
//   len — The length of the buffer resp. the length of the read password
//         string not including the terminating null byte.
// Returns 0 on success; -1 in case of a failure.
static int tui_ask_password(char *passwd, size_t *len) {
  filter(); // only affect the current line
  initscr();
  cbreak(); // don't cache the characters
  noecho(); // don't print the password
  keypad(stdscr, TRUE);

  printw("Enter the master password: ");

  size_t curr_len = 0;
  int c;
  for (; (c = getch()) != EOF && c != '\n' && curr_len < *len; ++curr_len) {
    passwd[curr_len] = (char)c;
  }
  passwd[curr_len] = '\0';

  *len = curr_len;

  endwin();

  if (c == EOF)
    return -1;
  return 0;
}
