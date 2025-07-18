#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "macro.h"
#include "symbol.h"
#include "memory.h"
#include "directive.h"
#include "instruction.h"
#include "utils.h"
#include "pass.h"

/* Maximum length for a line in the source file, including null terminator */
#define MAX_LINE_LENGTH 81

/* Define boolean type for ANSI C */
#define bool int
#define true 1
#define false 0


/* Represents a line in memory (instruction or data) */
/* MOVED TO MEMORY */


/* Struct for symbol table entry */
/* MOVED TO SYMBOL */

/* Struct for data values (.data and .string) */
typedef struct DataNode {
    int address;
    int value;
    struct DataNode *next;
} DataNode;

/* Struct for code instructions */
typedef struct InstructionNode {
    int address;
    char line[MAX_LINE_LENGTH];
    struct InstructionNode *next;
} InstructionNode;

/* ADDED TO MACRO.C AND MACRO.H */


/* Memory table (array of memory words) */
/* MOVED TO MEMORY */

/* Symbol table */
/* MOVED TO SYMBOL */

/* Instruction list */
InstructionNode *instruction_head = NULL;
InstructionNode *instruction_tail = NULL;

/* ADDED TO MACRO.C AND MACRO.H */







/* Checks if a line starts with a label (ends with a colon) */
/* MOVED TO UTILS */

/* Checks if the line contains a directive like .data or .string */
/* IS_DIRECTIVE MOVED TO DIRECTIVE */

/* Checks if the line contains a valid instruction */
/* MOVED TO INSTRUCTION.C */

/* Detects addressing mode of an operand */
/* Detects addressing mode of an operand */
/* MOVED TO INSTRUCTION.C */

/* Parses an instruction line and prints its parts */
/* MOVED TO INSTRUCTION.C */


/* Encodes a single assembly instruction into an integer machine word, Returns the encoded value as an int. */
/* MOVED TO INSTRUCTION.C */

/* PARSE_DATA_DIRECTIVE MOVED TO DIRECTIVE.C */
/* FIRST_PASS AND SECOND_PASS MOVES TO PASS.C */

/* Goes over the file again and handles .entry lines */
/* MARK_ENTRIES MOVED TO SYMBOL */

/* Creates the .ent file and writes all entry labels */
/* CREATE_ENTRY_FILE MOVED TO SYMBOL.C */

/* Writes the .ext file that lists where external labels were used */
/* WRITE_EXT_FILE MOVED TO SYMBOL.C */

/* Creates the .ob file containing the memory image (code + data) */
/* CREATE_OB_FILE MOVED TO MEMORY */



/* PRINT_MEMORY MOVED TO MEMORY */

/* MOVED TO SYMBOL */


int main(int argc, char *argv[]) {
    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char pre_filename[FILENAME_MAX];
    char am_filename[FILENAME_MAX];

    /* If the user has not given a file name */
    if (argc < 2) {
        printf("Usage: %s <filename.as>\n", argv[0]);
        return 1;
    }

    /* Step 1: Generate .pre and .am filenames */
    strncpy(pre_filename, argv[1], FILENAME_MAX);
    pre_filename[FILENAME_MAX - 1] = '\0';
    char *dot = strrchr(pre_filename, '.');
    if (dot != NULL) {
        strcpy(dot, ".pre");
    } else {
        strcat(pre_filename, ".pre");
    }

    strncpy(am_filename, argv[1], FILENAME_MAX);
    am_filename[FILENAME_MAX - 1] = '\0';
    dot = strrchr(am_filename, '.');
    if (dot != NULL) {
        strcpy(dot, ".am");
    } else {
        strcat(am_filename, ".am");
    }

    /* Step 2: Run preprocessor to handle macros */
    preprocess_file(argv[1], pre_filename);

    /* Step 3: Expand macro usages and write to .am file */
    expand_macros(pre_filename, am_filename);

    /* Step 4: Open .am file for reading */
    fp = fopen(am_filename, "r");
    if (!fp) {
        perror("Error opening .am file");
        return 1;
    }

    /* Step 5: First Pass - build symbol table and initial code/data */
    rewind(fp);
    first_pass(fp);

    /* Step 6: Second Pass - finalize entries, externs, and output files */
    second_pass(fp, argv[1]);

    /* Debug info */
    print_memory();
    print_symbol_table();

    fclose(fp);
    return 0;
}

