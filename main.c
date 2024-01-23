#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "keymaps.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
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

typedef struct Line {
  char *text;
  size_t len;
} Line;

typedef struct Lines {
  usint capacity, n;
  Line *data;
} Lines;

typedef struct Config {
  termios initialConfig;
  usint nRows, nCols;
  Lines text;
} Config;

Config config;

void addLine(Lines *lines, Line line) {
  if (lines->n < lines->capacity) {
    lines->data[lines->n] = line;
    lines->n++;
    return;
  }
  lines->capacity *= 2;
  Line *newLines = (Line *)malloc(lines->capacity * sizeof(Line));
  for (size_t i = 0; i < lines->n; ++i) {
    newLines[i] = lines->data[i];
  }
  newLines[lines->n] = line;
  lines->n++;
  free(lines->data);
  lines->data = newLines;
}

char *renderLine(const Line line) {
  char buff[USHRT_MAX] = {0};
  char *text = line.text;
  ssize_t chunk = config.nCols;
  size_t length = line.len;
  size_t buffSize = (chunk + strlen(NEXTLINE)) * ((size_t)length / chunk + 1);
  char *ans = malloc(buffSize * sizeof(char));
  ans[0] = '\0';

  if (length < chunk) {
    strcat(ans, text);
    return ans;
  }
  for (size_t i = 0; i < length - chunk; i += chunk) {
    memcpy(buff, text + i, chunk);
    buff[chunk] = '\0';
    strcat(ans, buff);
    strcat(ans, NEXTLINE);
  }
  size_t tail = length % chunk ? length % chunk : chunk;
  memcpy(buff, text + length - tail, tail);
  buff[tail] = '\0';
  strcat(ans, buff);
  return ans;
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

void parseFile() {
  char *fileName = "test.c";
  FILE *fp = fopen(fileName, "r");
  char *line = NULL;
  size_t buffLen, charsRead;

  if (fp == NULL) {
    exit(EXIT_FAILURE);
  }
  while ((charsRead = getline(&line, &buffLen, fp)) != -1) {
    write(STDOUT_FILENO, line, charsRead);
    write(STDOUT_FILENO, NEXTLINE, strlen(NEXTLINE));
    Line newLine = {.len = charsRead, .text = line};
    char *line = renderLine(newLine);
    // write(STDOUT_FILENO, line, strlen(line));
    addLine(&config.text, newLine);
    free(line);
    line = NULL;
  }

  fclose(fp);
}

void init() {
  memset(&config, 0, sizeof(config));
  config.text.capacity = 4;
  config.text.n = 0;
  config.text.data = malloc(config.text.capacity * sizeof(Line));
  terminalMakeRaw();
  getTerminalSize();
  resetScreen();
  setStdinFD();
  registerWINCHandler();
  parseFile();
  atexit(restoreTerminal);
}

int main() {
  init();

  char k = '\0';

  char *line = renderLine(config.text.data[0]);
  // write(STDOUT_FILENO, line, strlen(line));

  while (true) {
    ssize_t bytesRead = read(STDIN_FILENO, &k, 1);
    char buf[30];
    if (bytesRead != 1)
      continue;
    if (k == 'q')
      break;
  }
  return 0;
}
