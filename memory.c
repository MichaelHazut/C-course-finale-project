#include "memory.h"

/* Allocate global memory and initialize counter */
MemoryWord memory[MAX_MEMORY];
int memory_counter = MEM_START;

void create_ob_file(const char *original_filename)
{
    FILE *ob_fp;
    char ob_filename[FILENAME_MAX];
    int i, code_count = 0, data_count = 0;

    /* Build “foo.ob” from “foo.as” */
    strncpy(ob_filename, original_filename, FILENAME_MAX);
    ob_filename[FILENAME_MAX - 1] = '\0';
    {
        char *dot = strrchr(ob_filename, '.');
        if (dot) strcpy(dot, ".ob");
        else     strcat(ob_filename, ".ob");
    }

    ob_fp = fopen(ob_filename, "w");
    if (!ob_fp) {
        printf("Error: could not create %s\n", ob_filename);
        return;
    }

    /* Count how many code vs data words we have */
    for (i = MEM_START; i < memory_counter; i++) {
        if (memory[i].is_code) code_count++;
        else                   data_count++;
    }

    /* Header: number of code words and data words */
    fprintf(ob_fp, "%d %d\n", code_count, data_count);

    /* Then each memory line: address value */
    for (i = MEM_START; i < memory_counter; i++) {
        fprintf(ob_fp, "%03d %d\n",
                memory[i].address,
                memory[i].value);
    }

    fclose(ob_fp);
}

void print_memory(void)
{
    int i;
    printf("\n--- Memory Content ---\n");
    for (i = MEM_START; i < memory_counter; i++) {
        printf("Address: %03d | Value: %d | Type: %s\n",
               memory[i].address,
               memory[i].value,
               memory[i].is_code ? "Code" : "Data");
    }
}
