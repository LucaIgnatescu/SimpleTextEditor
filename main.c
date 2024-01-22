#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "keymaps.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

typedef unsigned short int usint;
typedef struct termios termios;

typedef struct Line{
  char * text;
} Line;

typedef struct Config {
  termios initialConfig;
  usint nRows, nCols;
} Config;

Config config;


void renderLine(const Line * line){
  if (config.nCols == 0) return;
  const size_t chunk = config.nCols;
  char * buf = (char *) malloc((chunk + 1) * sizeof(char));
  // memset(buf, 0, 0);

  buf[chunk] = '\0';

  size_t length = strlen(line->text);
  
  for(size_t i = 0; i < length; i += chunk){
    memcpy(buf, line + i, chunk);
    write()
  }
}

void terminalMakeRaw() {
  system("tput smcup");
  tcgetattr(STDIN_FILENO, &config.initialConfig);
  termios raw = config.initialConfig;
  cfmakeraw(&raw);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void restoreTerminal() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &config.initialConfig);
  system("tput rmcup");
  printf("%d", errno);
}

void setStdinFD() {
  int flag = fcntl(STDIN_FILENO, F_GETFL, 0);
  if (flag < 0) {
    printf("Could not set standard out \n");
    exit(0);
  }
  flag |= O_NONBLOCK;
  int ok = fcntl(STDIN_FILENO, F_SETFL, flag);
  if (ok < 0) {
    printf("Could not set standard out \n");
    exit(0);
  }
  printf("Set standard out" NEXTLINE);
}

void resetScreen() {
  char *buf = ERASESCREEN "" HOME;
  write(STDOUT_FILENO, buf, strlen(buf));
}

void getTerminalSize() {
  struct winsize ws;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1)
    exit(1);
  config.nRows = ws.ws_row;
  config.nCols = ws.ws_col;
}

void handleWINCH(int) {
  getTerminalSize();
  const char buf[100];
  sprintf((char *)buf, "%d %d" NEXTLINE, config.nRows, config.nCols);
  write(STDOUT_FILENO, buf, strlen(buf));
}

void registerWINCHandler() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = handleWINCH;
  sigaction(SIGWINCH, &sa, NULL);
}

void parseFile(){

}

void init() {
  memset(&config, 0, sizeof(config));
  terminalMakeRaw();
  getTerminalSize();
  resetScreen();
  setStdinFD();
  registerWINCHandler();
  atexit(restoreTerminal);
}

int main() {
  init();

  char k = '\0';
  while (true) {
    ssize_t bytesRead = read(STDIN_FILENO, &k, 1);
    char buf[30];
    if (bytesRead == 1) {
      if (k == 'q')
        break;
      char buf[30];
      sprintf(buf, "%c" NEXTLINE, k);
      write(STDOUT_FILENO, buf, strlen(buf));
    }
  }
  return 0;
}
