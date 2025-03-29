#include <stdlib.h>

void *__wrap_rs_malloc(size_t size){
    return malloc(size);
}

void *__wrap_rs_calloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}

void *__wrap_rs_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    return realloc(ptr, size);
}

void __wrap_rs_free(void *ptr) {
    free(ptr);
}
