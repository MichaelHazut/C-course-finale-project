#ifndef MEMORY_H
#define MEMORY_H

#include <stdio.h>

/* Size of the entire memory image */
#define MAX_MEMORY 1024

/* Where code/data words start in memory */
#define MEM_START 100

/* One machine‐word in memory */
typedef struct {
    int address;   /* numeric address, e.g. 100, 101, … */
    int value;     /* encoded instruction or data value */
    int is_code;   /* 1 = instruction, 0 = data */
} MemoryWord;

/* Global memory array and next free slot */
extern MemoryWord memory[MAX_MEMORY];
extern int memory_counter;

/* Write the .ob file: counts code/data and dumps each word */
void create_ob_file(const char *original_filename);

/* Debug: print memory contents to stdout */
void print_memory(void);

#endif /* MEMORY_H */
