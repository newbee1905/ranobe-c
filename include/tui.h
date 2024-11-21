#ifndef TUI_H
#define TUI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>

static inline char getch();
static inline void clear_screen();
static inline void move_cursor(int row, int col);

#endif // _WIN32

#ifdef TUI_IMPLEMENTATION

#ifndef _WIN32

static inline char getch() {
	struct termios oldt, newt;
	char ch;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

static inline void clear_screen() {
#ifdef _WIN32
	system("cls");
#else
	printf("\033[2J\033[H");
#endif
}

static inline void move_cursor(int row, int col) {
	printf("\033[%d;%dH", row, col);
}

#endif // _WIN32

#endif // TUI_IMPLEMENTATION

#endif // TUI_H
