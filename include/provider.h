#ifndef PROVIDER_H
#define PROVIDER_H

#include <stdlib.h>

typedef char **(*list_func_t)(const provider_t *provider, size_t *count);
typedef char **(*search_func_t)(const provider_t *provider, const char *query, size_t *count);
typedef char **(*chapter_list_func_t)(const provider_t *provider, const char *novel_id, size_t *count);
typedef char *(*chapter_html_func_t)(const provider_t *provider, const char *chapter_id);
typedef char *(*parse_html_to_markdown_func_t)(const provider_t *provider, const char *html);
typedef void (*destroy_func_t)(provider_t *provider);

struct provider {
	char *name;
	char *base_url;

	list_func_t get_latest_list;
	search_func_t search_novels;
	chapter_list_func_t get_chapter_list;
	chapter_html_func_t get_chapter_html;
	parse_html_to_markdown_func_t parse_html_to_markdown;
	destroy_func_t destroy;
};
typedef struct provider provider_t;

#endif // PROVIDER_H
