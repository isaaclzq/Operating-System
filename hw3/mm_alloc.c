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
int meta_size = sizeof(struct alloc_chunk);


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

void reuse_and_alloc(void* ptr, size_t size){
	unsigned leftover = ((struct alloc_chunk*)ptr)->size - size;
	struct alloc_chunk* rest = ptr + meta_size + size;
	reuse(ptr, size);
	rest->next = ((struct alloc_chunk*)ptr)->next;
	((struct alloc_chunk*)ptr)->next = rest;
	rest->size = leftover;
	rest->free = 1;
	rest->prev = ptr;
	if (rest->next){
		rest->next->prev = rest;	
	}
}

void *mm_malloc(size_t size) {
    /* YOUR CODE HERE */
    if (size == 0){
    	return NULL;
    }
    if (chunk == NULL){
    	int succeed = init(size);
    	if (succeed){
    		return chunk->data;
    	} else {
    		return NULL;
    	}
    }
    // implementing first fit
    struct alloc_chunk* iter = chunk;
    printf("1\n");
	while (iter->free == 0 || iter->size < size){
		printf("into while loop\n");
		if (NULL == iter->next){
			printf("2\n");
			void* p = sbrk(size + meta_size);
			if ((void*) -1 == p){
				perror("sbrk");
				return NULL;
			}
			iter->next = (struct alloc_chunk*) p;
			iter->next->size = size;
			iter->next->free = 0;
			iter->next->next = NULL;
			iter->next->prev = iter;		
			memset(iter->next->data, 0, size);
			output_list();
		    return iter->next->data;
		}
		iter = iter->next;
	}
	if (iter->size + meta_size >= size + 2 * meta_size){
		printf("3\n");
		reuse_and_alloc(iter, size);
		output_list();
		return iter->data;
	}
	if (iter->size + meta_size >= size + meta_size && iter->size + meta_size < size + 2 * meta_size) {
		printf("4\n");
		reuse(iter, size);
		output_list();
		return iter->data; 
	}
    return iter->next->data;
}

void *mm_realloc(void *ptr, size_t size) {
    /* YOUR CODE HERE */
    return NULL;
}

void mm_free(void *ptr) {
    /* YOUR CODE HERE */
    if (NULL != ptr){
    	struct alloc_chunk* meta = ptr - meta_size;
    	meta->free = 1;
    	printf("before layout: \n");
    	output_list();
    	printf("\n");
    	coalesce();
    	printf("after layout: \n");
    	output_list();
    	printf("\n");
    }
}

void output_list(void){
	struct alloc_chunk* iter = chunk;
	while (iter != NULL){
		printf("current: %p, size: %ld, free: %d, prev: %p, next: %p\n", iter, iter->size, iter->free, iter->prev, iter->next);
		iter = iter->next;
	}
}

void coalesce(){
	struct alloc_chunk* start = chunk;
	while (start != NULL){
		if (start->free == 1){
			if (start->next && start->next->free == 1){
				start->size += meta_size + start->next->size;
				start->next = start->next->next;
				if (start->next){
					start->next->prev = start;
				}
			}
			if (!start->next){
				return;
			} else {
				start = start->next;	
			}
		} else {
			start = start->next;
		}
	}
}


// struct alloc_chunk {
// 	size_t size;
// 	int free;
// 	struct alloc_chunk* prev;
// 	struct alloc_chunk* next;
// 	char data[0];
// };







