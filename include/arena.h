#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>

#define KiB(bytes) ((size_t)bytes << 10)
#define MiB(bytes) ((size_t)bytes << 20)
#define GiB(bytes) ((size_t)bytes << 30)

static const size_t ARENA_ALIGN_SIZE = alignof(max_align_t);

struct arena {
	char *memory;
	size_t size;
	size_t capacity;
	char stack;
};
typedef struct arena arena_t;

static inline arena_t *__arena_create(size_t size);
static inline arena_t *__arena_create_from_stack(size_t size, void *memory);

static inline arena_t *arena_create(size_t size, void *buffer_memory);
static inline void *arena_alloc(arena_t *arena, size_t size);
static inline void arena_free(arena_t *arena, size_t size);
static inline void arena_clear(arena_t *arena);
static inline void arena_destroy(arena_t *arena);

// This can be considered a feature or an undefined behaviour
// Since I am not actually free the memory so the memory are
// still there  and stitll accessible before being overwritten
#define ARENA_SAVE_SIZE(arena)                                                                                         \
	do {                                                                                                                 \
	size_t __arena_prev_size__ = (arena)->size
#define ARENA_RESET_SIZE(arena)                                                                                        \
	((arena)->size = __arena_prev_size__);                                                                               \
	}                                                                                                                    \
	while (0)

#ifdef ARENA_IMPLEMENTATION

static inline arena_t *__arena_create(size_t size) {
	arena_t *arena = malloc(sizeof(arena_t));
	assert(arena);

	arena->memory   = malloc(size);
	arena->capacity = size;
	arena->size     = 0;

	assert(arena->memory);

	arena->stack = 0;

	return arena;
}

static inline arena_t *__arena_create_from_stack(size_t size, void *memory) {
	arena_t *arena = malloc(sizeof(arena_t));
	assert(arena);

	arena->memory   = memory;
	arena->capacity = size;
	arena->size     = 0;
	arena->stack    = 1;

	return arena;
}

static inline arena_t *arena_create(size_t size, void *buffer_memory) {
	if (buffer_memory) {
		return __arena_create_from_stack(size, buffer_memory);
	} else {
		return __arena_create(size);
	}
}

static inline void *arena_alloc(arena_t *arena, size_t size) {
	size_t current_address = (size_t)(arena->memory + arena->size);
	size_t padding         = (ARENA_ALIGN_SIZE - (current_address & (ARENA_ALIGN_SIZE - 1))) & (ARENA_ALIGN_SIZE - 1);

	assert(arena->capacity - arena->size >= size + padding && "Out of memory and dynamic expansion not implemented");

	char *allocated = arena->memory + arena->size + padding;

	arena->size += size + padding;

	return allocated;
}

static inline void arena_free(arena_t *arena, size_t size) {
	assert(arena->size >= size);

	arena->size -= size;
}

static inline void arena_clear(arena_t *arena) {
	arena->size = 0;
}

static inline void arena_destroy(arena_t *arena) {
	if (!arena->stack) {
		assert(arena->memory);
		free(arena->memory);
		arena->memory = NULL;
	}

	free(arena);
	arena = NULL;
}

#endif // ARENA_IMPLEMENTATION

#endif // ARENA_H
