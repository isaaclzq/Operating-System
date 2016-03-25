/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include "mm_alloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static struct alloc_chunk* chunk;
unsigned meta_size = sizeof(size_t)
					 + sizeof(int) 
					 + 2 * sizeof(struct alloc_chunk*);

void init(size_t size){
	void* p = sbrk(size + sizeof(struct alloc_chunk));
	if ((void*) -1 == p){
		perror("sbrk");
		exit(1);
	}
	chunk = (struct alloc_chunk*) p;
	chunk->size = size;
	chunk->free = 0;
	chunk->next = NULL;
	chunk->prev = NULL;
	memset(chunk->data, 0, size);
}

void *mm_malloc(size_t size) {
    /* YOUR CODE HERE */
    if (chunk == NULL){
    	init(size);
    	return chunk->data;
    }
    return NULL;
}

void *mm_realloc(void *ptr, size_t size) {
    /* YOUR CODE HERE */
    return NULL;
}

void mm_free(void *ptr) {
    /* YOUR CODE HERE */
    if (NULL != ptr){
    	struct alloc_chunk* chunk_left = (struct alloc_chunk*) ptr - sizeof(struct alloc_chunk);
    	chunk_left->free = 1;
    }
}









