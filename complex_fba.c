#include <stdio.h>
#include <stdint.h>

#define MEM_OFFSET 5
#define HEADER_SIZE 4
#define true 1
#define false 0
typedef uint8_t bool;
/*
Complex fba is a stack based memory allocator meant for faster dynamic allocations limited to the stack.
This allocator does consume some of the buffer's memory to track allocations without calling on the heap to increase speed at the cost of some memory.
Currently each allocation header is 4 bytes and has the following structure

0000 0000 0000 0000 0000 0000 0000 0  000
^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  ^~ ----------------
29 bits are dedicated to tracking allocation size       |
                                                        |
00 <-----------------------------------------------------
^~ 2 bits track small memory fragmentation that could occur between 2 allocations that cannot be tracked by adding a new header
ex. 10 total bytes free in front of header but only 8 bytes are allocated
--------------- ---------------------------------- -------------- -------------------------
|header (free)| |memory block (8 bytes requested)| |2 bytes left| |Next header (allocated)|                  
--------------- ---------------------------------- -------------- -------------------------
                                                   ^~~~~~~~~~~~~~ Here there are 2 bytes that have been cut off because there is no room for a header to track it, and no free headers to merge into. 
                                                   The 2 reserved bits track the size of the cut off memory which will be merged when the block is freed
0 <- final bit tracks whether memory block is allocated or free
*/
typedef struct FixedAllocator {
    unsigned cur_ptr;
    unsigned end_ptr;
    void* buffer;
} FixedAllocator;

FixedAllocator init_fba(void* buffer, unsigned bytes) {
    FixedAllocator fba = {
        .cur_ptr = 0,
        .end_ptr = bytes,
        .buffer = buffer,
    };
    return fba;
}
static bool is_allocated(const uint32_t* header) {
    return *header & 1;
}

// Following addition to calculate next_header is a mess but does this:
/*                                              (may not be present)
                                                ^^^^^^^^^^^^^^^^^^^^
                                                ||||||||||||||||||||
--------------------- ------------------------  -------------------------      -------------
|(void*) last_header| |Dedicated Memory      |  |Lost Memory < 4 bytes  |      |next_header|
--------------------- ------------------------  -------------------------      -------------
MEM_OFFSET------------^ *last_header >> 3----^  (*last_header >> 1) & 3-^ + 1 -^
*/  
static uint32_t align_next_header(uint32_t* header) {
    return MEM_OFFSET + (*header >> 3) + ((*header >> 1) & 3) + 1;
}

uint32_t fba_get_size(FixedAllocator* restrict fba, void* mem) {
    if (fba->buffer + MEM_OFFSET > mem || fba->buffer + fba->end_ptr < mem) {
        return 0;
    }
    else {
        uint32_t* header = (mem - MEM_OFFSET);
        return *header >> 3;
    }
}

static uint32_t* increment_header(FixedAllocator* restrict fba, uint32_t* last_header) {
    uint32_t* next_header = (void*) last_header + align_next_header(last_header);
//    if (is_allocated(next_header) == true && (void*) next_header + align_next_header(next_header) >= fba->buffer + fba->cur_ptr) { 
//        return 0;
//    }
//    else if (is_allocated(next_header) == true && (void*) next_header + align_next_header(next_header) >= fba->buffer + fba->end_ptr) {
//        return 0;
//    }
    return next_header;
    
}

static void merge_free_blocks(uint32_t* header1, uint32_t* header2) {
    //takes header sizes and lost memory fragments and merges them into one number
    uint32_t new_size = (*header1 >> 3) + (((*header1 >> 1) & 3) + *header2 >> 3) + ((*header2 >> 1) & 3) + HEADER_SIZE;
    *header2 = 0;
    *header1 = new_size << 3;
}
static void* fba_find_mem(FixedAllocator* restrict fba, unsigned bytes) {
    uint32_t* last_header = fba->buffer;
    uint32_t* current_header = increment_header(fba, last_header); 
    //is mostly working but double check the header merge math checks out
    while ((void*) current_header < fba->buffer + fba->cur_ptr && (void*) current_header < fba->buffer + fba->end_ptr) {
        printf("*last_header    = 0x%x\n", *last_header);
        printf("*current_header = 0x%x\n", *current_header);
        if (is_allocated(current_header) == false && is_allocated(last_header) == false) {
            merge_free_blocks(last_header, current_header);
            printf("last_header new size = %lu\n", *last_header >> 3);
            current_header = increment_header(fba, last_header);
        }
        last_header = current_header;
        current_header = increment_header(fba, last_header);
        
    }
    return 0;
}

void* fba_malloc(FixedAllocator* restrict fba, const unsigned bytes) {
    if (fba->buffer == 0) {
        return 0;
    }
    else if (fba->cur_ptr + HEADER_SIZE + bytes > fba->end_ptr) {
        void* return_pointer = fba_find_mem(fba, bytes);
        return return_pointer;
    } 
    else {
        uint32_t* header = (uint32_t*) (fba->buffer + fba->cur_ptr);
        *header = (bytes << 3) + 1;
        fba->cur_ptr += MEM_OFFSET;
        void* return_pointer = fba->buffer + fba->cur_ptr;
        fba->cur_ptr += bytes + 1;
        return return_pointer;
    }
}


void fba_free(FixedAllocator* restrict fba, void* mem) {
    if (fba->buffer + MEM_OFFSET > mem || fba->buffer + fba->end_ptr < mem) {
        return;
    }
    else {
        uint32_t* header = (mem - MEM_OFFSET);
        if ((*header & 1) == 1) {
            uint32_t size = *header >> 3;
            printf("%u\n", size % 8);
            switch (size % 8) {
                case 0:
                    for (uint64_t* byte = (uint64_t*) mem; byte < (uint64_t*) ((void*) mem + size); byte++) {
                        *byte = 0;
                    }
                    break;
                case 2:
                    for (uint16_t* byte = (uint16_t*) mem; byte < (uint16_t*) ((void*) mem + size); byte++) {
                        *byte = 0;
                    }
                    break;
                case 4:
                    for (uint32_t* byte = (uint32_t*) mem; byte < (uint32_t*) ((void*) mem + size); byte++) {
                        *byte = 0;
                    }
                    break;
                case 6:
                    for (uint16_t* byte = (uint16_t*) mem; byte < (uint16_t*) ((void*) mem + size); byte++) {
                        *byte = 0;
                    }
                    break;
                default:
                    for (uint8_t* byte = (uint8_t*) mem; byte < (uint8_t*) ((void*) mem + size); byte++) {
                        *byte = 0;
                    }
                    break;
            }
            *header -= 1; //turns off allocation bit
        }
    }
}

int main() {
    uint8_t buffer[100] = {0};
    FixedAllocator fba = init_fba(buffer, sizeof(buffer));
    int* array = fba_malloc(&fba, 10 * sizeof(int));
    short* array2 = fba_malloc(&fba, 10 * sizeof(short));
    printf("array2 size: %u\n", fba_get_size(&fba, array2));
    printf("array size: %u\n", fba_get_size(&fba, array));
    printf("buffer = %lu\n", buffer);
    printf("array = %lu\n", array);
    printf("array2 = %lu\n", array2);
//    int* array3 = fba_malloc(&fba, 11 * sizeof(int));
    printf("*array = 0x%x\n", *((uint32_t*)((void*) array - MEM_OFFSET)));
    printf("*array2 = 0x%x\n", *((uint32_t*)((void*) array2 - MEM_OFFSET)));
    fba_free(&fba, array);
    fba_free(&fba, array2);
    char* string = fba_malloc(&fba, 11 * sizeof(char));
    int* array3 = fba_malloc(&fba, 10 * sizeof(int));
    printf("*array = 0x%x\n", *((uint32_t*)((void*) array - MEM_OFFSET)));
    printf("*array2 = 0x%x\n", *((uint32_t*)((void*) array2 - MEM_OFFSET)));
    
    return 0;
}
