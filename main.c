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
    int i;
    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char pre_filename[FILENAME_MAX];
    char am_filename[FILENAME_MAX];

    if (argc < 2) {
        printf("Usage: %s <file1.as> [file2.as] [...]\n", argv[0]);
        return 1;
    }

    for (i = 1; i < argc; i++) {
        printf("\n*** Processing file: %s ***\n", argv[i]);

        /* Step 1: Create .pre and .am filenames */
        strncpy(pre_filename, argv[i], FILENAME_MAX);
        pre_filename[FILENAME_MAX - 1] = '\0';
        char *dot = strrchr(pre_filename, '.');
        if (dot != NULL) {
            strcpy(dot, ".pre");
        } else {
            strcat(pre_filename, ".pre");
        }

        strncpy(am_filename, argv[i], FILENAME_MAX);
        am_filename[FILENAME_MAX - 1] = '\0';
        dot = strrchr(am_filename, '.');
        if (dot != NULL) {
            strcpy(dot, ".am");
        } else {
            strcat(am_filename, ".am");
        }

        /* Step 2: Run preprocessor and macro expansion */
        preprocess_file(argv[i], pre_filename);
        expand_macros(pre_filename, am_filename);

        /* Step 3: Open the .am file */
        fp = fopen(am_filename, "r");
        if (!fp) {
            fprintf(stderr, "Error: Cannot open file %s\n", am_filename);
            continue;
        }

        /* Step 4: Run first and second passes */
        rewind(fp);
        first_pass(fp);

        rewind(fp); /* rewind again in case second pass needs the file */
        second_pass(fp, argv[i]);

        /* Debug info */
        print_memory();
        print_symbol_table();

        fclose(fp);
    }

    return 0;
}


