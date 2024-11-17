#ifndef REGEX_H
#define REGEX_H

#include <pcre.h>

struct regex_compiled {
	const char *file_path;
	pcre *re;
	pcre_extra *re_extra;
};
typedef struct regex_compiled regex_compiled_t;

#define DEFINE_REGEX(name, path)                                                                   \
	static regex_compiled_t name = {.file_path = path, .re = NULL, .re_extra = NULL};                \
	__attribute__((constructor)) static void name##_init__(void) {                                   \
		FILE *file = fopen(name.file_path, "rb");                                                      \
		if (!file) {                                                                                   \
			fprintf(stderr, "Failed to open precompiled regex file: %s", name.file_path);                \
			exit(1);                                                                                     \
		}                                                                                              \
		fseek(file, 0, SEEK_END);                                                                      \
		long size = ftell(file);                                                                       \
		rewind(file);                                                                                  \
		void *buffer = malloc(size);                                                                   \
		if (!buffer) {                                                                                 \
			fprintf(stderr, "Failed to allocate memory for regex");                                      \
			fclose(file);                                                                                \
			exit(1);                                                                                     \
		}                                                                                              \
		fread(buffer, size, 1, file);                                                                  \
		fclose(file);                                                                                  \
		name.re = (pcre *)buffer;                                                                      \
                                                                                                   \
		int rc = pcre_fullinfo(name.re, NULL, PCRE_INFO_SIZE, &size);                                  \
		if (rc != 0) {                                                                                 \
			fprintf(stderr, "Invalid compiled regex file: %s.\n", name.file_path);                       \
			free(buffer);                                                                                \
			exit(1);                                                                                     \
		}                                                                                              \
                                                                                                   \
		const char *error;                                                                             \
		name.re_extra = pcre_study(name.re, 0, &error);                                                \
		if (error) {                                                                                   \
			fprintf(stderr, "PCRE study error: %s\n", error);                                            \
			free(buffer);                                                                                \
			exit(1);                                                                                     \
		}                                                                                              \
	}                                                                                                \
	__attribute__((destructor)) static void name##_cleanup__(void) {                                 \
		if (name.re) {                                                                                 \
			free(name.re);                                                                               \
			name.re = NULL;                                                                              \
		}                                                                                              \
		if (name.re_extra) {                                                                           \
			free(name.re_extra);                                                                         \
			name.re_extra = NULL;                                                                        \
		}                                                                                              \
	}

#endif // REGEX_H
