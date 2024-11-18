#ifndef ARENA_STRING_H
#define ARENA_STRING_H

#include "arena.h"

static inline char *arena_strndup(arena_t *arena, const char *str, size_t n);
static inline int arena_asprintf(arena_t *arena, char **strp, const char *fmt, ...);

#ifdef ARENA_STRING_IMPLEMENTATION

static inline char *arena_strndup(arena_t *arena, const char *str, size_t n) {
	char *copy = (char *)arena_alloc(arena, n + 1);
	if (copy) {
		memcpy(copy, str, n);
		copy[n] = '\0';
	}
	return copy;
}

static inline int arena_asprintf(arena_t *arena, char **strp, const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	int size = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (size < 0) {
		return size;
	}

	char *buffer = (char *)arena_alloc(arena, size + 1);
	if (!buffer) {
		return -1;
	}

	va_start(args, fmt);
	int written = vsnprintf(buffer, size + 1, fmt, args);
	va_end(args);

	if (written < 0) {
		return written;
	}

	*strp = buffer;
	return written;
}

#endif // ARENA_STRING_IMPLEMENTATION

#endif // ARENA_STRING_H
