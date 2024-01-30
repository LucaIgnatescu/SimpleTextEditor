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
  usint lineOffset;
  Lines text;
  char *fileName;
} Config;

typedef struct Cursor {
  usint termRow, termCol, lineIndex, lineOffset;
} Cursor;

Config config;
Cursor cursor;

void draw();

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
  draw();
}

void registerWINCHandler() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = handleWINCH;
  sigaction(SIGWINCH, &sa, NULL);
}

void parseFile() {
  char *fileName = "test.txt";
  FILE *fp = fopen(fileName, "r");
  char *line = NULL;
  size_t buffLen, charsRead;
  config.fileName = fileName;

  if (fp == NULL) {
    exit(EXIT_FAILURE);
  }
  while ((charsRead = getline(&line, &buffLen, fp)) != -1) {
    line[charsRead - 1] = '\0';
    Line newLine = {.len = charsRead - 1, .text = line};
    addLine(&config.text, newLine);
    line = NULL;
  }

  fclose(fp);
}

void draw() {
  write(STDOUT_FILENO, SAVECURSOR, 3);
  resetScreen();
  char *buf = (char *)malloc(config.nCols * config.nRows * 8 * sizeof(char));
  buf[0] = '\0';
  usint rows = config.nRows - 1;
  size_t range = rows + config.lineOffset > config.text.n
                     ? config.text.n - config.lineOffset
                     : rows;
  range = config.text.n > config.lineOffset ? range : 0;
  size_t overflow = 0;
  for (size_t i = 0; i < range; ++i) {
    Line line = config.text.data[i + config.lineOffset];
    // char *parsedLine = renderLine(line);
    char lineNum[10];
    size_t numLines = line.len / config.nCols;
    if (line.len % config.nCols != 0)
      numLines += 1;
    else if (line.len == 0)
      numLines = 1;
    overflow += numLines - 1;
    strcat(buf, line.text);
    strcat(buf, lineNum);
    strcat(buf, NEXTLINE);
    // change with memcpy for linear runtime
  }

  for (size_t i = range; i < rows; ++i) {
    strcat(buf, "~" NEXTLINE);
  }
  char lastLine[2 * config.nCols + 1];
  char *setColor = "\x1b[38;5;51m";
  sprintf(lastLine, "%stest.txt | %hu lines | %ld \x1b[0m", setColor,
          config.text.n, overflow); // Add more data
  strcat(buf, lastLine);
  write(STDOUT_FILENO, buf, strlen(buf));
  write(STDOUT_FILENO, RESTORECURSOR, 3);
}

void init() {
  memset(&config, 0, sizeof(config));
  memset(&cursor, 0, sizeof(cursor));
  config.text.capacity = 4;
  config.text.n = 0;
  config.text.data = malloc(config.text.capacity * sizeof(Line));
  terminalMakeRaw();
  getTerminalSize();
  setStdinFD();
  registerWINCHandler();
  parseFile();
  draw();
  write(STDOUT_FILENO, HOME, 4);
  atexit(restoreTerminal);
}

void processKeypress(char k) {
  if (k == 'q')
    exit(0);

  switch (k) {
    /** Movement **/
    case 'j':
      if (cursor.lineIndex == config.text.n - 1) {
        return;
      }
      if (cursor.termRow < config.nRows - 2) {
        write(STDOUT_FILENO, DOWN, 4);
        cursor.lineIndex++;
        cursor.termRow++;
      } else{
        config.lineOffset++;
        cursor.lineIndex++;
        draw();
      }
      break;
    case 'h':
      write(STDOUT_FILENO, LEFT, 4);
      break;
    case 'k':
      if(cursor.lineIndex == 0) return;
      if (cursor.termRow > 0){
       write(STDOUT_FILENO, UP, 4);
        cursor.termRow --;
        cursor.lineIndex --;
      } else if(config.lineOffset > 0){
        config.lineOffset --;
        cursor.lineIndex --;
        draw();
      }
      break;
    case 'l':
      write(STDOUT_FILENO, RIGHT, 4);
      break;
    }
}
int main() {
  init();
  char k = '\0';
  while (true) {
    ssize_t bytesRead = read(STDIN_FILENO, &k, 1);
    char buf[30];
    if (bytesRead != 1)
      continue;
    processKeypress(k);
  }
  return 0;
}
