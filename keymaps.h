#ifndef KEYMAPS
#define KEYMAPS

// Macros from https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797

#define UP "\x1b[1A"
#define DOWN "\x1b[1B"
#define RIGHT "\x1b[1C"
#define LEFT "\x1b[1D"
#define HOME "\x1b[H"

#define NEXTLINE "\x1b[1E"
#define PREVLINE "\x1b[1F"

#define ERASESCREEN "\x1b[2J"
#define ERASELINE "\x1b[2K"

#define RESETTEXT "\x1b[0m"
#define COLOR256F(color) "\x1b[38;5;" #color "m"
#define COLOR256B(color) "\x1b[48;5;" #color "m"

#endif // !KEYMAPS
