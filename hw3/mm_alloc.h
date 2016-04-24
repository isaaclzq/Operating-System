/*
 * mm_alloc.h
 *
 * A clone of the interface documented in "man 3 malloc".
 */

#pragma once

#include <stdlib.h>

void *mm_malloc(size_t size);
void *mm_realloc(void *ptr, size_t size);
void mm_free(void *ptr);

struct alloc_chunk {
	size_t size;
	int free;
	struct alloc_chunk* prev;
	struct alloc_chunk* next;
	char data[0];
};

int init(size_t size);
void reuse(struct alloc_chunk*, size_t);
void reuse_and_alloc(void* ptr, size_t size);
void output_list(void);
void coalesce(void);