#include <stdio.h>
#include <stdlib.h>
#include <pcre.h>
#include <string.h>

int main(int argc, char** argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <regex_file> <output_file>\n", argv[0]);
		return 1;
	}

	const char* regex_file = argv[1];
	const char* output_file = argv[2];

	FILE* input = fopen(regex_file, "r");
	if (!input) {
		fprintf(stderr, "Failed to open regex file");
		return 1;
	}

	fseek(input, 0, SEEK_END);
	long size = ftell(input);
	rewind(input);

	char* pattern = malloc(size + 1);
	if (!pattern) {
		fprintf(stderr, "Failed to allocate memory for regex pattern");
		fclose(input);
		return 1;
	}

	fread(pattern, size, 1, input);
	fclose(input);
	pattern[size] = '\0';

	size_t len = strlen(pattern);
	if (len > 0 && pattern[len - 1] == '\n') {
		pattern[len - 1] = '\0';
	}

	const char* error;
	int erroffset;
	pcre* re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
	free(pattern);
	if (!re) {
		fprintf(stderr, "PCRE compilation failed at offset %d: %s\n", erroffset, error);
		return 1;
	}

	FILE* output = fopen(output_file, "wb");
	if (!output) {
		fprintf(stderr, "Failed to open output file");
		pcre_free(re);
		return 1;
	}

	int compiled_size;
	if (pcre_fullinfo(re, NULL, PCRE_INFO_SIZE, &compiled_size) != 0) {
		fprintf(stderr, "Failed to get compiled regex size\n");
		fclose(output);
		pcre_free(re);
		return 1;
	}

	fwrite(re, compiled_size, 1, output);
	fclose(output);
	pcre_free(re);

	printf("Regex compiled and saved to %s\n", output_file);
	return 0;
}

