#include "keymaps.h"
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

// Very long line that should be read as extremely long so that it wraps around
// in the editor that would be nice.

typedef unsigned short int usint;

void draw();

class Line {
public:
  Line(const std::string &line) : text(line){};

private:
  std::string text;
  friend std::ostream &operator<<(std::ostream &, const Line &);
};

struct EditorConfig {
  termios initalConfig;
  usint nRows = 0, nCols = 0;
  usint lineOffset = 0, rowOffset = 0;
  usint rowOverflow = 0;
  std::vector<Line> lines;
} config;

std::ostream &operator<<(std::ostream &os, const Line &line) {
  std::stringstream ss(line.text);
  char *buffer = new char[config.nCols + 2];
  int overflow = -1;
  while (ss.get(buffer, config.nCols)) {
    if (overflow != -1) {
      os << NEXTLINE;
    }
    overflow += 1;
    os << buffer;
  }
  // os<<config.nCols;
  if (overflow > 0) {
    config.rowOverflow += overflow;
  }
  delete[] buffer;
  return os;
}

std::ostream &operator<<(std::ostream &os, const EditorConfig &config) {
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
  cfmakeraw(&raw);

  tcsetattr(STDERR_FILENO, TCSAFLUSH, &raw);
}

void getTerminalSize() {
  winsize w;
  ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
  config.nCols = w.ws_col;
  config.nRows = w.ws_row;
}

void handleWINCH(int) {
  getTerminalSize();
  draw();
}

void parseFile(const std::string &file) {
  std::ifstream inputFile(file);
  if (!inputFile.is_open()) {
    std::cout << "could not open file" << NEXTLINE;
    return;
  }

  std::string line;
  while (std::getline(inputFile, line)) {
    config.lines.push_back(line);
  }
  inputFile.close();
}

void draw() {
  std::cout << ERASESCREEN << HOME << std::flush;
  std::cout << config.nRows;
  // for (usint i = config.lineOffset;
  //      i < config.nRows + config.lineOffset - config.rowOverflow - 3; ++i) {
  //   std::cout << config.lines[i] << NEXTLINE;
  // }
}

void registerHandler() {
  struct sigaction resize;
  memset(&resize, '\0', sizeof(resize));
  resize.sa_handler = handleWINCH;
  sigaction(SIGWINCH, &resize, NULL);
}

void init() {
  std::cout << std::unitbuf << HOME << ERASESCREEN <<std::flush;
  enableRawMode();
  getTerminalSize();
  registerHandler();
  parseFile("main.cpp");
}

int main() {

  init();
  // draw();
  char key;
  while (std::cin.get(key) && key != 'q') {
    std::cout << "input" << NEXTLINE;
    if (key == 'x') {
      std::cout << ERASESCREEN;
    }
    if (key == 'd') {
      draw();
    }
    if (key == 'c') {
      std::cout << CLEAR;
    }
    if (key == 'h') {
      std::cout << HOME;
    }
  }
  return 0;
}
