#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
typedef struct ArenaAllocator {
    void* pointer_array;
    uintptr_t cur_ptr;
    uintptr_t len;
} ArenaAllocator;

ArenaAllocator init_arena(size_t arena_bytes) {
    ArenaAllocator arena = {
        .pointer_array = calloc(arena_bytes, sizeof(uint8_t)),
        .cur_ptr = (uintptr_t) arena.pointer_array,
        .len = (uintptr_t) arena.pointer_array + arena_bytes,
    };
    return arena;
}

void* arena_malloc(ArenaAllocator* restrict arena, size_t len) {
    if (arena->cur_ptr + len >= arena->len) {
        return 0;
    }
    void* return_pointer = (void*) arena->cur_ptr;
    arena->cur_ptr = arena->cur_ptr + len;
    return return_pointer;
}

void arena_deinit(ArenaAllocator* restrict arena) {
    if (arena->pointer_array != 0) {
        free(arena->pointer_array);
        arena->len = 0;
        arena->cur_ptr = 0;
        arena->pointer_array = 0;
    }
}
int main(void) {
    ArenaAllocator arena = init_arena(1000);
    int* array1 = (int*) arena_malloc(&arena, sizeof(int) * 10);
    int* array2 = (int*) arena_malloc(&arena, sizeof(int) * 10);
    char* string = (char*) arena_malloc(&arena, sizeof(char) * 50);
    strncpy(string, "This is a sample string!", 50);
    puts("array1 values:");
    for (int i = 0; i < 10; i++) {
        array1[i] = (i + 4) * 2;
    }
    
    for (int i = 0; i < 10; i++) {
        printf("%d\n", array1[i]);
    }
    puts("array2 values:");
    for (int i = 0; i < 10; i++) {
        array2[i] = (i + 7) * 2;
    }
    
    for (int i = 0; i < 10; i++) {
        printf("%d\n", array2[i]);
    }
    puts("test string:");
    printf("%s\n", string);
    printf("%s\n", string);
    uint64_t* oversized_value = (uint64_t*) arena_malloc(&arena, 100000000000000000 * sizeof(uint64_t));
    puts("oversized value:");
    printf("%lu\n", oversized_value);
    arena_deinit(&arena);
    return 0;
}

