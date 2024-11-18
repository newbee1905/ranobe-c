#ifndef HTML_UTILS_H
#define HTML_UTILS_H

#include <string.h>

struct html_entity {
	const char *entity;
	unsigned long codepoint;
};
typedef struct html_entity html_entity_t;

static const html_entity_t entities[] = {
	{  "&quot;", 0x0022},
  {  "&apos;", 0x0027},
  {   "&amp;", 0x0026},
  {    "&lt;", 0x003C},
  {    "&gt;", 0x003E},
	{  "&nbsp;", 0x00A0},
  {  "&copy;", 0x00A9},
  {   "&reg;", 0x00AE},
  {  "&euro;", 0x20AC},
  { "&pound;", 0x00A3},
	{  "&cent;", 0x00A2},
  {   "&deg;", 0x00B0},
  {"&middot;", 0x00B7},
  {  "&bull;", 0x2022},
  { "&mdash;", 0x2014},
	{ "&ndash;", 0x2013},
  {"&hellip;", 0x2026},
  { "&ldquo;", 0x201C},
  { "&rdquo;", 0x201D},
  {      NULL,      0}  // Sentinel value
};

static inline unsigned long hex_to_int(const char *hex, size_t len);
static inline unsigned long dec_to_int(const char *dec);
static inline size_t unicode_to_utf8(unsigned long codepoint, char *buffer);
static inline int is_unicode_escape(const char *str);
int html_decode(const char *input, char *output);

#ifdef HTML_UTILS_IMPEMENTATION

static inline unsigned long hex_to_int(const char *hex, size_t len) {
	unsigned long result = 0;
	for (size_t i = 0; i < len && hex[i]; i++) {
		result *= 16;
		char c = hex[i];
		if (isdigit(c)) {
			result += c - '0';
		} else if (c >= 'a' && c <= 'f') {
			result += c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			result += c - 'A' + 10;
		}
	}
	return result;
}

static inline unsigned long dec_to_int(const char *dec) {
	unsigned long result = 0;
	while (*dec && isdigit(*dec)) {
		result = result * 10 + (*dec - '0');
		dec++;
	}
	return result;
}

static inline size_t unicode_to_utf8(unsigned long codepoint, char *buffer) {
	if (codepoint <= 0x7F) {
		buffer[0] = (char)codepoint;
		return 1;
	} else if (codepoint <= 0x7FF) {
		buffer[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
		buffer[1] = 0x80 | (codepoint & 0x3F);
		return 2;
	} else if (codepoint <= 0xFFFF) {
		buffer[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
		buffer[1] = 0x80 | ((codepoint >> 6) & 0x3F);
		buffer[2] = 0x80 | (codepoint & 0x3F);
		return 3;
	} else if (codepoint <= 0x10FFFF) {
		buffer[0] = 0xF0 | ((codepoint >> 18) & 0x07);
		buffer[1] = 0x80 | ((codepoint >> 12) & 0x3F);
		buffer[2] = 0x80 | ((codepoint >> 6) & 0x3F);
		buffer[3] = 0x80 | (codepoint & 0x3F);
		return 4;
	}
	return 0; // Invalid codepoint
}

static inline int is_unicode_escape(const char *str) {
	return (str[0] == '\\' && (str[1] == 'u' || str[1] == 'U'));
}

int html_decode(const char *input, char *output) {
	if (!input || !output) {
		return 0;
	}

	char *write_ptr      = output;
	const char *read_ptr = input;

	while (*read_ptr) {
		// Handle Unicode escape sequences (\uXXXX or \UXXXXXXXX)
		if (is_unicode_escape(read_ptr)) {
			char is_long   = (read_ptr[1] == 'U');
			size_t hex_len = is_long ? 8 : 4;

			if (strlen(read_ptr + 2) >= hex_len) {
				unsigned long codepoint = hex_to_int(read_ptr + 2, hex_len);
				if (codepoint > 0) {
					write_ptr += unicode_to_utf8(codepoint, write_ptr);
					read_ptr += 2 + hex_len;
					continue;
				}
			}
		}
		// Handle HTML entities
		else if (*read_ptr == '&') {
			// Check for numeric entities
			if (*(read_ptr + 1) == '#') {
				unsigned long codepoint = 0;
				const char *end         = NULL;

				if (*(read_ptr + 2) == 'x' || *(read_ptr + 2) == 'X') {
					// Hexadecimal entity
					char hex[10] = {0};
					end          = strchr(read_ptr + 3, ';');
					if (end && end - (read_ptr + 3) < 9) {
						strncpy(hex, read_ptr + 3, end - (read_ptr + 3));
						codepoint = hex_to_int(hex, 8);
					}
				} else if (isdigit(*(read_ptr + 2))) {
					// Decimal entity
					char dec[10] = {0};
					end          = strchr(read_ptr + 2, ';');
					if (end && end - (read_ptr + 2) < 9) {
						strncpy(dec, read_ptr + 2, end - (read_ptr + 2));
						codepoint = dec_to_int(dec);
					}
				}

				if (end && codepoint > 0) {
					write_ptr += unicode_to_utf8(codepoint, write_ptr);
					read_ptr = end + 1;
					continue;
				}
			}

			// Check named entities
			int found = 0;
			for (int i = 0; entities[i].entity != NULL; i++) {
				size_t len = strlen(entities[i].entity);
				if (strncmp(read_ptr, entities[i].entity, len) == 0) {
					write_ptr += unicode_to_utf8(entities[i].codepoint, write_ptr);
					read_ptr += len;
					found = 1;
					break;
				}
			}
			if (found) {
				continue;
			}
		}

		*write_ptr++ = *read_ptr++;
	}

	*write_ptr = '\0';
	return 1;
}

#endif // HTML_UTILS_IMPEMENTATION

#endif // HTML_UTILS_H
