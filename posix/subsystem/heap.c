#include <mm/alloc.h>

void *calloc(size_t num, size_t size) {
    void *ptr = malloc(num * size);
    if (ptr)
        memset(ptr, 0, num * size);
    return ptr;
}

char *strdup(const char *s) {
    size_t len = strlen((char *)s);
    char *ptr = (char *)malloc(len + 1);
    if (ptr == NULL)
        return NULL;
    memcpy(ptr, (void *)s, len + 1);
    return ptr;
}
