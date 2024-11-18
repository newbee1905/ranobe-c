#include <ctype.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h> /* for realloc */
#include <string.h> /* for memcpy */
#include <time.h>
#include <wchar.h>

#include <pcre.h>

#define CURL_UTILS_IMPEMENTATION
#include "curl_utils.h"

#define HTML_UTILS_IMPEMENTATION
#include "html_utils.h"

#define REGEX_IMPLEMENTATION
#include "regex.h"

struct novel_info {
	char *name;
	char *url;
};
typedef struct novel_info novel_info_t;

struct chapter_info {
	char *name;
	char *url;
};
typedef struct chapter_info chapter_info_t;

DEFINE_REGEX(novel_list_, "compiled_regex/animedaily/novel_list.pcre");
DEFINE_REGEX(novel_search_list_, "compiled_regex/animedaily/novel_search_list.pcre");
DEFINE_REGEX(novel_id_, "compiled_regex/animedaily/novel_id.pcre");
DEFINE_REGEX(pagination_list_, "compiled_regex/animedaily/pagination_list.pcre");
DEFINE_REGEX(chapter_list_, "compiled_regex/animedaily/chapter_list.pcre");
DEFINE_REGEX(novel_content_, "compiled_regex/animedaily/novel_content.pcre");

signed main(void) {
	srand(time(NULL));

	int status = 0;

	CURL *curl;
	CURLcode res;
	memory_t chunk = {0};
	long res_code;
	size_t res_length;

	novel_info_t novels[64];
	novel_info_t chapters[64];
	size_t novel_size   = 0;
	size_t chapter_size = 0;

	res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res != CURLE_OK) {
		fprintf(stderr, "Failed global init ...\n");
		exit(1);
	}

	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "curl_easy_init() failed");
	}

	if (curl_get(curl, "https://animedaily.net/front", &chunk) != CURL_GET_OK) {
		fprintf(stderr, "Request failed\n");
		status = 1;
		goto clean_up;
	}

	regex_match_t match;
	regex_match_init(&match);

	for (regex_input_t input = {.s = chunk.res, .len = strlen(chunk.res), .pos = 0};
	     regex_match_next(&novel_list_, &input, &match) == REGEX_MATCH_OK; regex_match_cleanup(&match)) {

		novels[novel_size].url    = strndup(match.captures[1], match.capture_lens[1]);
		novels[novel_size++].name = strndup(match.captures[2], match.capture_lens[2]);
	}
	memory_free(&chunk);

	// printf("Captured %zu novels\n", novel_size);
	// for (size_t i = 0; i < novel_size; ++i) {
	// 	printf("Name: %s\n", novels[i].name);
	// 	printf("URL: %s\n", novels[i].url);
	// }

	if (curl_get(curl, novels[0].url, &chunk) != CURL_GET_OK) {
		fprintf(stderr, "Request failed\n");
		status = 1;
		goto clean_up;
	}

	printf("%s\n", novels[0].url);

	char *id;

	for (regex_input_t input = {.s = chunk.res, .len = strlen(chunk.res), .pos = 0};
	     regex_match_next(&novel_id_, &input, &match) == REGEX_MATCH_OK; regex_match_cleanup(&match)) {
		id = strndup(match.captures[1], match.capture_lens[1]);
	}

	printf("%s\n", id);

	// assuming longest page number is 4 digits
	char *last_page_str = malloc(sizeof(char) * 4);
	if (!last_page_str) {
		fprintf(stderr, "Failed to alloc last page string\n");
		status = 1;
		goto clean_up;
	}

	for (regex_input_t input = {.s = chunk.res, .len = strlen(chunk.res), .pos = 0};
	     regex_match_next(&pagination_list_, &input, &match) == REGEX_MATCH_OK; regex_match_cleanup(&match)) {
		strncpy(last_page_str, match.captures[1], match.capture_lens[1]);
	}
	memory_free(&chunk);

	long last_page = strtol(last_page_str, NULL, 10);
	free(last_page_str);
	printf("%ld\n", last_page);

	char post_data[64];
	sprintf(post_data, "action=tw_ajax&type=pagination&id=%s&page=%ld", id, 1l);
	if (curl_post(curl, "https://animedaily.net/wp-admin/admin-ajax.php", post_data, &chunk) != CURL_GET_OK) {
		fprintf(stderr, "Request failed\n");
		status = 1;
		goto clean_up;
	}

	for (regex_input_t input = {.s = chunk.res, .len = strlen(chunk.res), .pos = 0};
	     regex_match_next(&chapter_list_, &input, &match) == REGEX_MATCH_OK; regex_match_cleanup(&match)) {

		char *host             = strndup(match.captures[1], match.capture_lens[1]);
		char *novel_name       = strndup(match.captures[2], match.capture_lens[2]);
		char *chapter_name_url = strndup(match.captures[3], match.capture_lens[3]);

		char *url;
		asprintf(&url, "https://%s/%s/%s", host, novel_name, chapter_name_url);
		free(host);
		free(novel_name);
		free(chapter_name_url);

		char *__chapter_name = strndup(match.captures[4], match.capture_lens[4]);
		char *chapter_name   = html_decode(__chapter_name);
		free(__chapter_name);

		chapters[chapter_size].url    = url;
		chapters[chapter_size++].name = chapter_name;
	}
	memory_free(&chunk);

	// printf("Captured %zu chapters\n", chapter_size);
	// for (size_t i = 0; i < chapter_size; ++i) {
	// 	printf("Name: %s\n", chapters[i].name);
	// 	printf("URL: %s\n", chapters[i].url);
	// }

	if (curl_get(curl, chapters[0].url, &chunk) != CURL_GET_OK) {
		fprintf(stderr, "Request failed\n");
		status = 1;
		goto clean_up;
	}

	char content[8192];
	size_t content_size = 0;

	for (regex_input_t input = {.s = chunk.res, .len = strlen(chunk.res), .pos = 0};
	     regex_match_next(&novel_content_, &input, &match) == REGEX_MATCH_OK; regex_match_cleanup(&match)) {
		switch (*(match.captures[0] + 1)) {
		case 'd':
			content[content_size++] = '\n';
			content[content_size++] = '\n';
			break;
		case 'p':
			strncpy(content + content_size, match.captures[1], match.capture_lens[1]);
			content_size += match.capture_lens[1];
			break;
		}

		printf("%ld\n", content_size);
	}
	content[content_size] = '\0';
	memory_free(&chunk);

	printf("%s\n", content);

clean_up:
	for (size_t i = 0; i < novel_size; ++i) {
		if (novels[i].name) {
			free(novels[i].name);
			novels[i].name = NULL;
		}
		if (novels[i].url) {
			free(novels[i].url);
			novels[i].url = NULL;
		}
	}

	if (id) {
		free(id);
		id = NULL;
	}

	for (size_t i = 0; i < chapter_size; ++i) {
		if (chapters[i].name) {
			free(chapters[i].name);
			chapters[i].name = NULL;
		}
		if (chapters[i].url) {
			free(chapters[i].url);
			chapters[i].url = NULL;
		}
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	return status;
}