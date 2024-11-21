#ifndef RANOBE_UTILS_H
#define RANOBE_UTILS_H

#include "tui.h"

static inline char get_vim_arrow_key(char ch);

#ifdef RANOBE_UTILS_IMPLEMENTATION

static inline char get_vim_arrow_key(char ch) {
#ifdef _WIN32
	if (ch == -32) {
		ch = getch();
		switch (ch) {
		case 72:
			return 'k';
		case 75:
			return 'h';
		case 77:
			return 'l';
		case 80:
			return 'j';
		}
	}
#else
	if (ch == '\033') {
		getch();
		ch = getch();
		switch (ch) {
		case 'A':
			return 'k';
		case 'B':
			return 'j';
		case 'D':
			return 'h';
		case 'C':
			return 'l';
		}
	}
#endif

	return ch;
}

#endif // RANOBE_UTILS_IMPLEMENTATION

#endif // RANOBE_UTILS_H
