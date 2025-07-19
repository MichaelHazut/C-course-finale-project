#include "main.h"

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

/* Instruction list */
InstructionNode *instruction_head = NULL;
InstructionNode *instruction_tail = NULL;


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

