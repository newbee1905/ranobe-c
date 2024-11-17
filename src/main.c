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

	int rc;
	int ovector[30];

	novel_info_t novels[64];
	novel_info_t chapters[64];
	size_t novel_size = 0;
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

	res_length = strlen(chunk.res);

	for (const char *cur_pos = chunk.res;
	     (rc = pcre_exec(novel_list_.re, novel_list_.re_extra, cur_pos, res_length, 0, 0, ovector, 30)
	     ) >= 0;
	     cur_pos += ovector[1], res_length -= ovector[1]) {
		if (novel_size >= 64) {
			fprintf(stderr, "Exceeded novel array capacity\n");
			break;
		}

		novels[novel_size].url    = strndup(cur_pos + ovector[2], ovector[3] - ovector[2]);
		novels[novel_size++].name = strndup(cur_pos + ovector[4], ovector[5] - ovector[4]);
	}

	// printf("Captured %zu novels\n", novel_size);
	// for (size_t i = 0; i < novel_size; ++i) {
	//  printf("Name: %s\n", novels[i].name);
	//  printf("URL: %s\n", novels[i].url);
	// }

	if (curl_get(curl, novels[0].url, &chunk) != CURL_GET_OK) {
		fprintf(stderr, "Request failed\n");
		status = 1;
		goto clean_up;
	}

	printf("%s\n", novels[0].url);

	char *id;
	res_length = strlen(chunk.res);
	if ((rc = pcre_exec(novel_id_.re, novel_id_.re_extra, chunk.res, res_length, 0, 0, ovector, 30)) >= 0) {
		id = strndup(chunk.res + ovector[2], ovector[3] - ovector[2]);
	}
	printf("%s\n", id);

	res_length = strlen(chunk.res);

	char *last_page_str;
	for (const char *cur_pos = chunk.res;
	     (rc = pcre_exec(
					pagination_list_.re, pagination_list_.re_extra, cur_pos, res_length, 0, 0, ovector, 30
				)) >= 0;
	     cur_pos += ovector[1], res_length -= ovector[1]) {

		last_page_str = strndup(cur_pos + ovector[2], ovector[3] - ovector[2]);
	}

	long last_page = strtol(last_page_str, NULL, 10);
	printf("%ld\n", last_page);

	char post_data[64];
	sprintf(post_data, "action=tw_ajax&type=pagination&id=%s&page=%ld", id, 1l);
	if (curl_post(curl, "https://animedaily.net/wp-admin/admin-ajax.php", post_data, &chunk) != CURL_GET_OK) {
		fprintf(stderr, "Request failed\n");
		status = 1;
		goto clean_up;
	}

	for (const char *cur_pos = chunk.res;
	     (rc = pcre_exec(
					chapter_list_.re, chapter_list_.re_extra, cur_pos, res_length, 0, 0, ovector, 30
				)) >= 0;
	     cur_pos += ovector[1], res_length -= ovector[1]) {

		if (chapter_size >= 64) {
			fprintf(stderr, "Exceeded novel array capacity\n");
			break;
		}

		char *host             = strndup(cur_pos + ovector[2], ovector[3] - ovector[2]);
		char *novel_name       = strndup(cur_pos + ovector[4], ovector[5] - ovector[4]);
		char *chapter_name_url = strndup(cur_pos + ovector[6], ovector[7] - ovector[6]);

		char *url;
		asprintf(&url, "https://%s/%s/%s", host, novel_name, chapter_name_url);

		char *chapter_name = html_decode(strndup(cur_pos + ovector[8], ovector[9] - ovector[8]));

		chapters[chapter_size].url = url;
		chapters[chapter_size++].name = chapter_name;
	}

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

	res_length = strlen(chunk.res);

	char content[8192];
	size_t content_size = 0;

	for (const char *cur_pos = chunk.res;
	     (rc = pcre_exec(
					novel_content_.re, novel_content_.re_extra, cur_pos, res_length, 0, 0, ovector, 30
				)) >= 0;
	     cur_pos += ovector[1], res_length -= ovector[1]) {

		switch (*(cur_pos + ovector[0] + 1)) {
		case 'd':
			content[content_size++] = '\n';
			content[content_size++] = '\n';
			break;
		case 'p':
			strncpy(content + content_size, strndup(cur_pos + ovector[2], ovector[3] - ovector[2]), ovector[3] - ovector[2]);
			content_size += ovector[3] - ovector[2];
			break;
		}

		printf("%ld\n", content_size);
	}
	content[content_size] = '\0';

	printf("%s\n", content);

clean_up:
	memory_free(&chunk);
	curl_easy_cleanup(curl);
	curl_global_cleanup();

	return status;
}
