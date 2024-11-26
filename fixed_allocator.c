#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct FixedAllocator {
    unsigned cur_ptr;
    unsigned end_ptr;
    void* void_buffer;
} FixedAllocator;

FixedAllocator init_fba(void* buffer, size_t size) {
    FixedAllocator fba = {
        .cur_ptr = 0, 
        .end_ptr = (unsigned) size,
        .void_buffer = buffer,
    };
    return fba;
}

void* fba_malloc(FixedAllocator* fba, size_t bytes) {
    if (fba->cur_ptr + bytes - 1 > fba->end_ptr) {
        return 0;
    }
    else {
        void* return_pointer = fba->void_buffer + fba->cur_ptr;
        fba->cur_ptr += bytes + 1;
        return return_pointer;
    }
}

int main(void) {
    uint8_t mem_block[1000] = {0};
    printf("%d\n", sizeof(mem_block));
    FixedAllocator fba = init_fba(&mem_block, 1000); 
    char (*sentences)[22] = fba_malloc(&fba, 3 * 22);
    char* next_string = (char*) fba_malloc(&fba, 50);
    strncpy(sentences[2], "This is a sample string", 23);
    printf("%u\n", strlen(sentences[2]));
    strncpy(next_string, "This is a sample string!", 50);
    printf("%s\n", sentences[2]);
    printf("%s\n", next_string);
    int* array1 = (int*) fba_malloc(&fba, 100 * sizeof(int));
    if (array1 != 0) {
        for (register int i = 0; i < 50; i++) {
            array1[i] = i + 1;
        }
        for (register int i = 0; i < 50; i++) {
            printf("%d\n", array1[i]);
        }
    }
    else {
        puts("array1 was too big");
    }
    
}
