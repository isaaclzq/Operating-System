#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

/* Function pointers to hw3 functions */
void* (*mm_malloc)(size_t);
void* (*mm_realloc)(void*, size_t);
void (*mm_free)(void*);

void load_alloc_functions() {
    void *handle = dlopen("hw3lib.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    char* error;
    mm_malloc = dlsym(handle, "mm_malloc");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    mm_realloc = dlsym(handle, "mm_realloc");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    mm_free = dlsym(handle, "mm_free");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
}

int main() {
    load_alloc_functions();

    //testing reuse
    // int *data = (int*) mm_malloc(sizeof(int));
    // assert(data != NULL);
    // data[0] = 0x162;
    // mm_free(data);
    // int *data1 = (int*) mm_malloc(sizeof(int));
    // mm_free(data1);
    
    // testing malloc in a row
    // int *data = (int*) mm_malloc(4*sizeof(int));
    // int *data1 = (int*) mm_malloc(4*sizeof(int));
    // assert(data != NULL);
    // assert(data1 != NULL);
    // printf("first malloc succeed\n");
    // printf("second malloc succeed\n");
    // mm_free(data);
    // mm_free(data1);

    // testing splitting
    // int *A = (int*) mm_malloc(16*sizeof(int));
    // int *B = (int*) mm_malloc(4*sizeof(int));
    // assert(A != NULL);
    // assert(B != NULL);
    // printf("first malloc succeed\n");
    // printf("second malloc succeed\n");
    // mm_free(A);
    // int *C = (int*) mm_malloc(14*sizeof(int));
    // int *D = (int*) mm_malloc(2*sizeof(int));
    // mm_free(C);
    // mm_free(D);
    // mm_free(B);

    // testing merge
    int *A = (int*) mm_malloc(8*sizeof(int));
    int *B = (int*) mm_malloc(8*sizeof(int));
    assert(A != NULL);
    assert(B != NULL);
    mm_free(A);
    mm_free(B);
    int *C = (int*) mm_malloc(10*sizeof(int));
    mm_free(C);



    //printf("malloc test successful!\n");
    return 0;
}
