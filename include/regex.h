#ifndef REGEX_H
#define REGEX_H

#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>

#define OVECSIZE          32
#define REGEX_MATCH_OK    0
#define REGEX_MATCH_ERROR -1

struct regex_compiled {
	const char *file_path;
	pcre *re;
	pcre_extra *re_extra;
};
typedef struct regex_compiled regex_compiled_t;

struct regex_input {
	const char *s;
	size_t len;
	size_t pos;
};
typedef struct regex_input regex_input_t;

struct regex_match {
	int ovector[OVECSIZE];
	int rc;
	const char *match_start;
	int *capture_lens;
	char **captures;
	size_t num_captures;
	arena_t *arena;
};
typedef struct regex_match regex_match_t;

#define DEFINE_REGEX(name, path)                                                                                       \
	static regex_compiled_t name = {.file_path = path, .re = NULL, .re_extra = NULL};                                    \
	__attribute__((constructor)) static void name##_init__(void) {                                                       \
		FILE *file = fopen(name.file_path, "rb");                                                                          \
		if (!file) {                                                                                                       \
			fprintf(stderr, "Failed to open precompiled regex file: %s", name.file_path);                                    \
			exit(1);                                                                                                         \
		}                                                                                                                  \
		fseek(file, 0, SEEK_END);                                                                                          \
		long size = ftell(file);                                                                                           \
		rewind(file);                                                                                                      \
		void *buffer = malloc(size);                                                                                       \
		if (!buffer) {                                                                                                     \
			fprintf(stderr, "Failed to allocate memory for regex");                                                          \
			fclose(file);                                                                                                    \
			exit(1);                                                                                                         \
		}                                                                                                                  \
		fread(buffer, size, 1, file);                                                                                      \
		fclose(file);                                                                                                      \
		name.re = (pcre *)buffer;                                                                                          \
                                                                                                                       \
		int rc = pcre_fullinfo(name.re, NULL, PCRE_INFO_SIZE, &size);                                                      \
		if (rc != 0) {                                                                                                     \
			fprintf(stderr, "Invalid compiled regex file: %s.\n", name.file_path);                                           \
			free(buffer);                                                                                                    \
			exit(1);                                                                                                         \
		}                                                                                                                  \
                                                                                                                       \
		const char *error;                                                                                                 \
		name.re_extra = pcre_study(name.re, 0, &error);                                                                    \
		if (error) {                                                                                                       \
			fprintf(stderr, "PCRE study error: %s\n", error);                                                                \
			free(buffer);                                                                                                    \
			exit(1);                                                                                                         \
		}                                                                                                                  \
	}                                                                                                                    \
	__attribute__((destructor)) static void name##_cleanup__(void) {                                                     \
		if (name.re) {                                                                                                     \
			free(name.re);                                                                                                   \
			name.re = NULL;                                                                                                  \
		}                                                                                                                  \
		if (name.re_extra) {                                                                                               \
			free(name.re_extra);                                                                                             \
			name.re_extra = NULL;                                                                                            \
		}                                                                                                                  \
	}

static inline void regex_match_init(arena_t *arena, regex_match_t *match);
static inline void regex_match_cleanup(regex_match_t *match);
static inline int regex_extract_captures(regex_match_t *match, const char *input);
static inline int regex_match_next(regex_compiled_t *regex, regex_input_t *input, regex_match_t *match);

#ifdef REGEX_IMPLEMENTATION

static inline void regex_match_init(arena_t *arena, regex_match_t *match) {
	match->rc           = 0;
	match->match_start  = NULL;
	match->capture_lens = NULL;
	match->captures     = NULL;
	match->num_captures = 0;
	match->arena        = arena;
}

static inline void regex_match_cleanup(regex_match_t *match) {
	match->num_captures = 0;
}

static inline int regex_extract_captures(regex_match_t *match, const char *input) {
	if (match->rc < 1) {
		return REGEX_MATCH_ERROR;
	}

	match->num_captures = match->rc;
	match->capture_lens = arena_alloc(match->arena, sizeof(int) * match->num_captures);
	match->captures     = arena_alloc(match->arena, sizeof(char *) * match->num_captures);

	if (!match->capture_lens || !match->captures) {
		regex_match_cleanup(match);
		return REGEX_MATCH_ERROR;
	}

	for (size_t i = 0; i < match->num_captures; ++i) {
		int start              = match->ovector[2 * i];
		int end                = match->ovector[2 * i + 1];
		match->capture_lens[i] = end - start;

		match->captures[i] = arena_alloc(match->arena, match->capture_lens[i] + 1);
		if (!match->captures[i]) {
			regex_match_cleanup(match);
			return REGEX_MATCH_ERROR;
		}

		strncpy(match->captures[i], input + start, match->capture_lens[i]);
		match->captures[i][match->capture_lens[i]] = '\0';
	}

	return REGEX_MATCH_OK;
}

static inline int regex_match_next(regex_compiled_t *regex, regex_input_t *input, regex_match_t *match) {
	if (!regex || !input || !match || input->pos >= input->len) {
		return REGEX_MATCH_ERROR;
	}

	match->rc = pcre_exec(
		regex->re, regex->re_extra, input->s + input->pos, input->len - input->pos, 0, 0, match->ovector, OVECSIZE
	);

	if (match->rc < 0) {
		return REGEX_MATCH_ERROR;
	}

	match->match_start = input->s + input->pos;
	input->pos += match->ovector[1];

	return regex_extract_captures(match, match->match_start);
}

#endif // REGEX_IMPLEMENTATION

#endif // REGEX_H
