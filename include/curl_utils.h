#ifndef CURL_UTILS_H
#define CURL_UTILS_H

#define CURL_GET_OK 0

#include "arena.h"

const char *user_agents[] = {
	"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
	"Chrome/114.0.0.0 Safari/537.36",
	"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) "
	"Chrome/91.0.4472.124 Safari/537.36",
	"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 "
	"Safari/537.36"};

const char *referers[] = {"https://google.com", "https://bing.com", "https://duckduckgo.com"};

#define SET_RANDOM_USER_AGENT(curl)                                                                                    \
	do {                                                                                                                 \
		const char *random_user_agent = user_agents[rand() % (sizeof(user_agents) / sizeof(user_agents[0]))];              \
		curl_easy_setopt(curl, CURLOPT_USERAGENT, random_user_agent);                                                      \
	} while (0)

#define SET_RANDOM_REFERER(curl)                                                                                       \
	do {                                                                                                                 \
		const char *random_referer = referers[rand() % (sizeof(referers) / sizeof(referers[0]))];                          \
		curl_easy_setopt(curl, CURLOPT_AUTOREFERER, random_referer);                                                       \
	} while (0)

struct memory {
	char *res;
	size_t size;
	arena_t *arena;
};
typedef struct memory memory_t;

static inline void memory_init(arena_t *arena, memory_t *mem);
static inline void memory_free(memory_t *mem);
static inline void memory_move(memory_t *dst, memory_t *src);

static inline size_t __write_cb(char *data, size_t size, size_t nmemb, void *clientp);
int curl_get(CURL *curl, const char *url, memory_t *out);
int curl_post(CURL *curl, const char *url, const char *post_data, memory_t *out);

#ifdef CURL_UTILS_IMPEMENTATION

static inline void memory_init(arena_t *arena, memory_t *mem) {
	mem->res   = NULL;
	mem->size  = 0;
	mem->arena = arena;
}

static inline void memory_free(memory_t *mem) {
	mem->res  = NULL;
	mem->size = 0;
	arena_clear(mem->arena);
}

static inline void memory_move(memory_t *dst, memory_t *src) {
	dst->res  = src->res;
	dst->size = src->size;
	src->res  = NULL;
	src->size = 0;
}

static inline size_t __write_cb(char *data, size_t size, size_t nmemb, void *clientp) {
	size_t realsize = size * nmemb;
	memory_t *mem   = (memory_t *)clientp;

	// TODO: do a proper realloc for arena
	// This only expland the mem-res if the mem->res is the latest allocated
	// memory within the arena
	if (mem->res && (mem->res + mem->size == mem->arena->memory + mem->arena->size)) {
		if (mem->arena->size + realsize <= mem->arena->capacity) {
			mem->arena->size += realsize;
			memcpy(mem->res + mem->size, data, realsize);
			mem->size += realsize;
			mem->res[mem->size] = '\0';
			return realsize;
		}
	}

	char *ptr = arena_alloc(mem->arena, mem->size + realsize + 1);
	if (!ptr) {
		fprintf(stderr, "Failed to allocate memory for curl write callback\n");
		return 0;
	}

	if (mem->res) {
		memcpy(ptr, mem->res, mem->size);
	}

	mem->res = ptr;
	memcpy(&(mem->res[mem->size]), data, realsize);
	mem->size += realsize;
	mem->res[mem->size] = 0;

	return realsize;
}

int curl_get(CURL *curl, const char *url, memory_t *out) {
	if (!curl || !url || !out) {
		return -1;
	}

	memory_init(out->arena, out);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)out);
	curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	SET_RANDOM_USER_AGENT(curl);
	SET_RANDOM_REFERER(curl);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		memory_free(out);
		return -1;
	}

	long res_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);

	if ((res_code / 100) != 2) {
		fprintf(stderr, "GET %s: Unexpected res code: %ld\n", url, res_code);
		memory_free(out);
		return -1;
	}

	return 0;
}

int curl_post(CURL *curl, const char *url, const char *post_data, memory_t *out) {
	if (!curl || !url || !post_data || !out) {
		return -1;
	}

	memory_init(out->arena, out);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);              // Enable POST
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data); // Set POST data
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)out);
	curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	SET_RANDOM_USER_AGENT(curl);
	SET_RANDOM_REFERER(curl);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		memory_free(out);
		return -1;
	}

	long res_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);

	if ((res_code / 100) != 2) {
		fprintf(stderr, "POST %s: Unexpected res code: %ld\n", url, res_code);
		memory_free(out);
		return -1;
	}

	return 0;
}

#endif // CURL_UTILS_IMPEMENTATION

#endif // CURL_UTILS_H
