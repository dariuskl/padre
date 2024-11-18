// This is free and unencumbered software released into the public domain.

#include "nonstd.h"

void tui_set_invisible_mode(int fd) {
  print_to_file(utf8("\033[8m"), fd);
}

void tui_reset_invisible_mode(int fd) {
  print_to_file(utf8("\033[28m"), fd);
}

void tui_erase_previous_line(int fd) {
  print_to_file(utf8("\033[1A\033[2K" "Password entered." "\033[1B\r"), fd);
}

// Asks the user for his master password and stores it in `passwd`.
// Returns 0 on success; -1 in case of a failure.
void tui_ask_password(buf8 *passwd) {
  int tty_fd = open_file(utf8("/dev/tty"), 2);
  print_to_file(utf8("Enter the master password: "), tty_fd);

  tui_set_invisible_mode(tty_fd);
  while (scan_from_file(passwd, tty_fd) > 0 && *(passwd->eod - 1) != '\n') {
  }
  tui_reset_invisible_mode(tty_fd);
  tui_erase_previous_line(tty_fd);

  // remove the newline from the password
  --passwd->eod;
}
