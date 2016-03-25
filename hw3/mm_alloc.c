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
unsigned meta_size = sizeof(struct alloc_chunk);


int init(size_t size){
	void* p = sbrk(size + meta_size);
	if ((void*) -1 == p){
		perror("sbrk");
		return 0;
	}
	chunk = (struct alloc_chunk*) p;
	chunk->size = size;
	chunk->free = 0;
	chunk->next = NULL;
	chunk->prev = NULL;		
	memset(chunk->data, 0, size);
	return 1;
}

void reuse(struct alloc_chunk* ptr, size_t size){
	ptr->size = size;
	ptr->free = 0;
	memset(ptr->data, 0, size);
}

void reuse_and_alloc(struct alloc_chunk* ptr, size_t size){
	unsigned leftover = ptr->size - size;
	struct alloc_chunk* rest = ptr + meta_size + size;
	reuse(ptr, size);
	rest->next = ptr->next;
	ptr->next = rest;
	rest->size = leftover;
	rest->free = 1;
	rest->prev = ptr;
	rest->next->prev = rest;
}

void *mm_malloc(size_t size) {
    /* YOUR CODE HERE */
    if (chunk == NULL){
    	int succeed = init(size);
    	if (succeed){
    		return chunk->data;
    	} else {
    		return NULL;
    	}
    }
    // implementing first fit
 //    struct alloc_chunk* iter = chunk;
 //    while (iter->next != NULL){
 //    	while (iter->free == 0){
 //    		iter = iter->next;
 //    	}
 //    	if (iter->size - meta_size > size + meta_size){
 //    		reuse_and_alloc(iter, size);
 //    		return iter->data;
 //    	} else if (iter->size > size + meta_size) {
 //    		reuse(iter, size);
 //    		return iter->data; 
 //    	}
 //    	iter = iter->next;
 //    }
 //    void* p = sbrk(size + meta_size);
	// if ((void*) -1 == p){
	// 	perror("sbrk");
	// 	exit(1);
	// }
	// iter->next = (struct alloc_chunk*) p;
	// iter->next->size = size;
	// iter->next->free = 0;
	// iter->next->next = NULL;
	// iter->next->prev = iter;		
	// memset(iter->next->data, 0, size);
    //return iter->next->data;
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









