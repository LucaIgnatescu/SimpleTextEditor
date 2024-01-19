#include "keymaps.h"
#include <csignal>
#include <cstdio>
#include <fstream>
#include <ios>
#include <iostream>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

typedef unsigned short int usint;

class Line {
public:
  Line(const std::string &line) : text(line){};

private:
  std::string text;
  friend std::ostream &operator<<(std::ostream &, const Line &);
};

std::ostream &operator<<(std::ostream &os, const Line &line) {
  os << line.text; // Add wraparound
  return os;
}

struct EditorConfig {
  termios initalConfig;
  usint nRows = 0, nCols = 0;
  usint currLine = 0;
  std::vector<Line> lines;
} config;

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

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1;

  tcsetattr(STDERR_FILENO, TCSAFLUSH, &raw);
}

void getTerminalSize(int) {
  winsize w;
  ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
  config.nCols = w.ws_col;
  config.nRows = w.ws_row;
  config.lines.reserve(config.nRows);
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
  std::cout << ERASESCREEN;

  for (usint i = 0; i < config.nRows; ++i) {
    i += config.currLine;
    std::cout << config.lines[i] << NEXTLINE;
  }
}

void registerHandler(){
  struct sigaction resize;
  resize.sa_handler = getTerminalSize;
  sigaction(SIGWINCH, &resize, NULL);
}

void init(){
  std::cout << std::unitbuf << ERASESCREEN << HOME;
  enableRawMode();
  getTerminalSize(0);
  registerHandler();
  parseFile("main.cpp");
}


int main() {
  
  init();
  draw();

  char key;
  while (std::cin.get(key) && key != 'q') {
    std::cout << key;
    if (key == 'n') {
      std::cout << NEXTLINE;
    }
  }
  return 0;
}
