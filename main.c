#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Maximum length for a line in the source file, including null terminator */
#define MAX_LINE_LENGTH 81

/* Define boolean type for ANSI C */
#define bool int
#define true 1
#define false 0


/* Represents a line in memory (instruction or data) */
typedef struct {
    int address;        /* Memory address */
    int value;          /* Binary value */
    int is_code;        /* 1 = instruction, 0 = data */
} MemoryWord;

/* Struct for symbol table entry */
typedef struct Symbol {
    char name[MAX_LINE_LENGTH];
    int address;
    bool is_data;
    bool is_external;
    bool is_entry;
    struct Symbol *next;
} Symbol;

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


/* Memory table (array of memory words) */
#define MAX_MEMORY 1024
MemoryWord memory[MAX_MEMORY];
int memory_counter = 100;  /* Starts at 100 as per project specs */


/* Symbol table */
Symbol *symbol_table_head = NULL;

/* Instruction list */
InstructionNode *instruction_head = NULL;
InstructionNode *instruction_tail = NULL;


/* Checks if a line starts with a label (ends with a colon) */
bool is_label(const char *line) {
    const char *colon;
    const char *p;

    /* Skip leading spaces or tabs */
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    /* Look for a colon (':') which indicates a label */
    colon = strchr(line, ':');
    if (!colon) {
        return false;
    }

    /* Check that all characters before the ':' are alphanumeric */
    for (p = line; p < colon; p++) {
        if (!isalnum(*p)) {
            return false;
        }
    }

    return true;
}


/* Checks if the line contains a directive like .data or .string */
bool is_directive(const char *line) {
    const char *p;

    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    /* If there's a label, skip past it */
    p = strchr(line, ':');
    if (p) {
        line = p + 1;
        while (*line == ' ' || *line == '\t') {
            line++;
        }
    }

    /* Check for known directives */
    if (strncmp(line, ".data", 5) == 0 ||
        strncmp(line, ".string", 7) == 0 ||
        strncmp(line, ".extern", 7) == 0 ||
        strncmp(line, ".entry", 6) == 0) {
        return true;
        }

    return false;
}


/* Checks if the line contains a valid instruction */
bool is_instruction(const char *line) {
    const char *p;

    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    /* If there's a label, skip past it */
    p = strchr(line, ':');
    if (p) {
        line = p + 1;
        while (*line == ' ' || *line == '\t') {
            line++;
        }
    }

    /* Check for known instructions */
    if (strncmp(line, "mov", 3) == 0 ||
        strncmp(line, "add", 3) == 0 ||
        strncmp(line, "sub", 3) == 0 ||
        strncmp(line, "cmp", 3) == 0 ||
        strncmp(line, "lea", 3) == 0 ||
        strncmp(line, "clr", 3) == 0 ||
        strncmp(line, "not", 3) == 0 ||
        strncmp(line, "inc", 3) == 0 ||
        strncmp(line, "dec", 3) == 0 ||
        strncmp(line, "jmp", 3) == 0 ||
        strncmp(line, "bne", 3) == 0 ||
        strncmp(line, "jsr", 3) == 0 ||
        strncmp(line, "red", 3) == 0 ||
        strncmp(line, "prn", 3) == 0 ||
        strncmp(line, "rts", 3) == 0 ||
        strncmp(line, "stop", 4) == 0) {
        return true;
        }

    return false;
}


/* Detects addressing mode of an operand */
int detect_addressing_mode(const char *operand) {
    if (!operand) return -1; /* No operand */

    /* Immediate addressing: starts with '#' */
    if (operand[0] == '#') {
        return 0;
    }

    /* Register addressing: starts with 'r' followed by digit (e.g., r3) */
    if (operand[0] == 'r' && isdigit(operand[1]) && operand[2] == '\0') {
        return 3;
    }

    /* Index addressing: contains brackets like LABEL[r2] */
    if (strchr(operand, '[') && strchr(operand, ']')) {
        return 2;
    }

    /* Otherwise it's direct addressing */
    return 1;
}


/* Parses an instruction line and prints its parts */
void parse_instruction(const char *line) {
    char copy[MAX_LINE_LENGTH];
    char *label = NULL;
    char *opcode;
    char *operand1 = NULL;
    char *operand2 = NULL;

    /* Make a copy because strtok modifies the string */
    strncpy(copy, line, MAX_LINE_LENGTH);
    copy[MAX_LINE_LENGTH - 1] = '\0';

    /* If there's a label, skip it */
    char *colon = strchr(copy, ':');
    if (colon) {
        label = strtok(copy, ":");
        opcode = strtok(NULL, " \t\n");
    } else {
        opcode = strtok(copy, " \t\n");
    }

    /* Get first operand */
    operand1 = strtok(NULL, ", \t\n");

    /* Get second operand (if exists) */
    operand2 = strtok(NULL, ", \t\n");

    printf("==> Instruction parsed:\n");
    printf("    Opcode: %s\n", opcode);

    if (operand1) {
        printf("    Operand 1: %s\n", operand1);
        printf("    Addressing mode 1: %d\n", detect_addressing_mode(operand1));
    }

    if (operand2) {
        printf("    Operand 2: %s\n", operand2);
        printf("    Addressing mode 2: %d\n", detect_addressing_mode(operand2));
    }
}


void first_pass(FILE *fp) {
    char line[MAX_LINE_LENGTH];
    char *line_ptr;
    int line_number = 0;

    rewind(fp);

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        Symbol *new_sym;
        InstructionNode *new_instr;
        char label_name[MAX_LINE_LENGTH];

        int is_label_line;
        int is_dir;
        int is_instr;

        line_number++;

        /* Skip empty or comment lines */
        if (line[0] == '\n' || line[0] == ';') {
            continue;
        }

        /* Start from the beginning of line */
        line_ptr = line;

        /* Trim leading spaces */
        while (isspace(*line_ptr)) {
            line_ptr++;
        }

        is_label_line = is_label(line_ptr);
        is_dir = is_directive(line_ptr);
        is_instr = is_instruction(line_ptr);

        /* If the line has a label, extract and register it */
        if (is_label_line) {
            sscanf(line_ptr, "%[^:]:", label_name);

            new_sym = (Symbol *)malloc(sizeof(Symbol));
            if (!new_sym) {
                printf("Memory allocation error at line %d\n", line_number);
                continue;
            }

            strcpy(new_sym->name, label_name);
            new_sym->address = memory_counter;
            new_sym->is_entry = 0;
            new_sym->is_external = 0;
            new_sym->is_data = is_dir;
            new_sym->next = symbol_table_head;
            symbol_table_head = new_sym;

            /* Move pointer after label */
            line_ptr = strchr(line_ptr, ':');
            if (line_ptr != NULL) {
                line_ptr++;
                while (isspace(*line_ptr)) {
                    line_ptr++;
                }
            }
        }

        /* If it's a directive (.data / .string) */
        if (is_dir) {
            /* TODO: implement parse_data_directive(line_ptr); */
        }
        /* If it's an instruction */
        else if (is_instr) {
            new_instr = (InstructionNode *)malloc(sizeof(InstructionNode));
            if (!new_instr) {
                printf("Memory allocation error at line %d\n", line_number);
                continue;
            }

            new_instr->address = memory_counter;
            strncpy(new_instr->line, line_ptr, MAX_LINE_LENGTH);
            new_instr->line[MAX_LINE_LENGTH - 1] = '\0';
            new_instr->next = NULL;

            /* Add to instruction list */
            if (instruction_head == NULL) {
                instruction_head = new_instr;
                instruction_tail = new_instr;
            } else {
                instruction_tail->next = new_instr;
                instruction_tail = new_instr;
            }

            memory_counter++; /* Each instruction takes 1 word (basic assumption) */
        }
    }
}





int main(int argc, char *argv[]) {
    FILE *fp;
    char line[MAX_LINE_LENGTH];

    /* If the user has not given a file name */
    if (argc < 2) {
        printf("Usage: %s <filename.as>\n", argv[0]);
        return 1;
    }

    /* Open the file in read mode */
    fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    /* First Pass: Print full contents of the file */
    printf("\n1) Full contents of the file:\n");

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        printf("%s", line);
    }

    /* Second Pass: Analyze each line */
    rewind(fp);
    printf("\n\n\n2) Line analysis:");

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        printf("\n>> Line: | %s", line);

        bool has_label = is_label(line);
        bool has_directive = is_directive(line);
        bool has_instruction = is_instruction(line);

        if (has_label) {
            printf("=> Type: Label\n");
        }
        if (has_directive) {
            printf("=> Type: Directive\n");
        }
        if (has_instruction) {
            printf("=> Type: Instruction\n");
            parse_instruction(line);
        }
        if (!has_label && !has_directive && !has_instruction) {
            printf("=> Type: Unknown or empty line\n");
        }


    }

    fclose(fp);
    return 0;
}
