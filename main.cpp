#include "keymaps.h"
#include <asm-generic/ioctls.h>
#include <csignal>
#include <ios>
#include <iostream>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

typedef unsigned short int usint;

struct editorConfig {
  termios initalConfig;
  usint nRows, nCols;

} config;

std::ostream &operator<<(std::ostream &os, const editorConfig &config) {
  os << config.nRows << " " << config.nCols;
  return os;
}

void disableRawMode() {
  tcsetattr(STDERR_FILENO, TCSANOW, &config.initalConfig);
}

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &config.initalConfig);
  atexit(disableRawMode);

  termios raw = config.initalConfig;

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1;

  tcsetattr(STDERR_FILENO, TCSAFLUSH, &raw);
}

void getTerminalSize(int signal) {
  if (signal != SIGWINCH)
    return;

  winsize w;
  ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
  config.nCols = w.ws_col;
  config.nRows = w.ws_row;
  std::cout << config << "\n";
}

int main() {
  std::cout << std::unitbuf;

  enableRawMode();

  struct sigaction resize;
  resize.sa_handler = getTerminalSize;
  sigaction(SIGWINCH, &resize, NULL);

  char key;

  while (std::cin.get(key) && key != 'q') {
    std::cout << key;
  }

  return 0;
}
