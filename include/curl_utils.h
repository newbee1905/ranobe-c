#ifndef CURL_UTILS_H
#define CURL_UTILS_H

#define CURL_GET_OK 0

struct memory {
	char *res;
	size_t size;
};
typedef struct memory memory_t;

static inline void memory_init(memory_t *mem);
static inline void memory_free(memory_t *mem);
static inline void memory_move(memory_t *dst, memory_t *src);

static inline size_t __write_cb(char *data, size_t size, size_t nmemb, void *clientp);
int curl_get(CURL *curl, const char *url, memory_t *out);

#ifdef CURL_UTILS_IMPEMENTATION

static inline void memory_init(memory_t *mem) {
	mem->res  = NULL;
	mem->size = 0;
}

static inline void memory_free(memory_t *mem) {
	if (mem->res) {
		free(mem->res);
		mem->res = NULL;
	}
	mem->size = 0;
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

	char *ptr = realloc(mem->res, mem->size + realsize + 1);
	if (!ptr) {
		fprintf(stderr, "Failed to allocate memory for curl write callback\n");
		return 0;
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

	memory_init(out);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)out);
	curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		memory_free(out);
		exit(1);
	}

	return 0;
}

#endif // CURL_UTILS_IMPEMENTATION

#endif // CURL_UTILS_H
