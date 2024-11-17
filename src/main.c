#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h> /* for realloc */
#include <string.h> /* for memcpy */
#include <time.h>

#include <pcre.h>

#define CURL_UTILS_IMPEMENTATION
#include "curl_utils.h"

#include "regex.h"

struct novel_info {
	char *name;
	char *url;
};
typedef struct novel_info novel_info_t;

DEFINE_REGEX(novel_list_, "compiled_regex/animedaily/novel_list.pcre");
DEFINE_REGEX(novel_search_list_, "compiled_regex/animedaily/novel_search_list.pcre");

signed main(void) {
	int status = 0;

	CURL *curl;
	CURLcode res;
	memory_t chunk = {0};
	long res_code;
	size_t res_length;

	int rc;
	int ovector[30];

	novel_info_t novels[32];
	size_t size = 0;

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
		curl_easy_cleanup(curl);
		curl_global_cleanup();
		exit(1);
	}
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);

	if (res_code != 200) {
		fprintf(stderr, "Unexpected res code: %ld\n", res_code);
		curl_easy_cleanup(curl);
		curl_global_cleanup();
		exit(1);
	}

	res_length = strlen(chunk.res);

	for (const char *cur_pos = chunk.res;
	     (rc = pcre_exec(novel_list_.re, novel_list_.re_extra, cur_pos, res_length, 0, 0, ovector, 30)
	     ) >= 0;
	     cur_pos += ovector[1], res_length -= ovector[1]) {
		if (size >= 32) {
			fprintf(stderr, "Exceeded novel array capacity\n");
			break;
		}

		novels[size].url    = strndup(cur_pos + ovector[2], ovector[3] - ovector[2]);
		novels[size++].name = strndup(cur_pos + ovector[4], ovector[5] - ovector[4]);
	}

	printf("Captured %zu novels\n", size);
	for (size_t i = 0; i < size; ++i) { // only print out 4 novels to check
		printf("Name: %s\n", novels[i].name);
		printf("URL: %s\n", novels[i].url);
	}

	if (curl_get(curl, "https://animedaily.net/?s=hero", &chunk) != CURL_GET_OK) {
		fprintf(stderr, "Request failed\n");
		status = 1;
		goto clean_up;
	}
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);

	if (res_code != 200) {
		fprintf(stderr, "Unexpected res code: %ld\n", res_code);
		status = 1;
		goto clean_up;
	}

	res_length = strlen(chunk.res);
	size       = 0;

	for (const char *cur_pos = chunk.res;
	     (rc = pcre_exec(
					novel_search_list_.re, novel_search_list_.re_extra, cur_pos, res_length, 0, 0, ovector, 30
				)) >= 0;
	     cur_pos += ovector[1], res_length -= ovector[1]) {
		if (size >= 32) {
			fprintf(stderr, "Exceeded novel array capacity\n");
			break;
		}

		novels[size].url    = strndup(cur_pos + ovector[2], ovector[3] - ovector[2]);
		novels[size++].name = strndup(cur_pos + ovector[4], ovector[5] - ovector[4]);
	}

	printf("Captured %zu novels\n", size);
	for (size_t i = 0; i < size; ++i) { // only print out 4 novels to check
		printf("Name: %s\n", novels[i].name);
		printf("URL: %s\n", novels[i].url);
	}

clean_up:
	memory_free(&chunk);
	curl_easy_cleanup(curl);
	curl_global_cleanup();

	return status;
}
