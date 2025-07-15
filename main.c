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

typedef struct Macro {
    char name[MAX_LINE_LENGTH];
    char content[MAX_LINE_LENGTH * 10];
    struct Macro *next;
} Macro;

/* Macro table */
Macro *macro_table = NULL;

/* Memory table (array of memory words) */
#define MAX_MEMORY 1024
MemoryWord memory[MAX_MEMORY];
int memory_counter = 100;  /* Starts at 100 as per project specs */


/* Symbol table */
Symbol *symbol_table_head = NULL;

/* Instruction list */
InstructionNode *instruction_head = NULL;
InstructionNode *instruction_tail = NULL;


void add_macro(const char *name, const char *content) {
    Macro *new_macro = (Macro *)malloc(sizeof(Macro));
    if (!new_macro) {
        printf("Error: memory allocation failed for macro.\n");
        return;
    }

    strcpy(new_macro->name, name);
    strcpy(new_macro->content, content);
    new_macro->next = macro_table;
    macro_table = new_macro;
}


void preprocess_file(const char *input_filename, const char *output_filename) {
    FILE *input_fp = fopen(input_filename, "r");
    FILE *output_fp = fopen(output_filename, "w");
    char line[MAX_LINE_LENGTH];
    char macro_name[MAX_LINE_LENGTH];
    char macro_content[MAX_LINE_LENGTH * 10];
    int in_macro = 0;

    if (!input_fp || !output_fp) {
        printf("Error: failed to open file(s).\n");
        return;
    }

    while (fgets(line, MAX_LINE_LENGTH, input_fp)) {
        if (strncmp(line, "macro", 5) == 0) {
            in_macro = 1;
            sscanf(line, "macro %s", macro_name);
            macro_content[0] = '\0';  /* Clear macro content buffer */
            continue;
        }

        if (in_macro) {
            if (strncmp(line, "endmacro", 8) == 0) {
                in_macro = 0;
                add_macro(macro_name, macro_content);
                continue;
            }
            strcat(macro_content, "\t");
            strcat(macro_content, line);  /* Append line to macro content */
        } else {
            fprintf(output_fp, "%s", line);  /* Regular line, write as-is */
        }
    }

    fclose(input_fp);
    fclose(output_fp);
}

void expand_macros(const char *input_filename, const char *output_filename) {
    FILE *input_fp = fopen(input_filename, "r");
    FILE *output_fp = fopen(output_filename, "w");
    char line[MAX_LINE_LENGTH];
    Macro *curr_macro;

    if (!input_fp || !output_fp) {
        printf("Error: failed to open file(s) in expand_macros.\n");
        return;
    }

    while (fgets(line, MAX_LINE_LENGTH, input_fp)) {
        int matched = 0;

        /* Trim leading spaces */
        char *trimmed = line;
        while (isspace(*trimmed)) trimmed++;

        /* Try to match with a macro name */
        curr_macro = macro_table;
        while (curr_macro != NULL) {
            if (strncmp(trimmed, curr_macro->name, strlen(curr_macro->name)) == 0 &&
                isspace(trimmed[strlen(curr_macro->name)]) || trimmed[strlen(curr_macro->name)] == '\n') {
                fprintf(output_fp, "%s", curr_macro->content);
                matched = 1;
                break;
                }
            curr_macro = curr_macro->next;
        }

        if (!matched) {
            fprintf(output_fp, "%s", line);  /* Not a macro, write as-is */
        }
    }

    fclose(input_fp);
    fclose(output_fp);
}


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

/* Encodes a single assembly instruction into an integer machine word, Returns the encoded value as an int. */
int encode_instruction(const char *line) {
    char copy[MAX_LINE_LENGTH];
    char *opcode_str, *operand1 = NULL, *operand2 = NULL;
    int opcode = 0;
    int src_mode = 0, dst_mode = 0;
    int instruction_word = 0;

    strncpy(copy, line, MAX_LINE_LENGTH);
    copy[MAX_LINE_LENGTH - 1] = '\0';

    /* Skip label if exists */
    char *colon = strchr(copy, ':');
    if (colon) {
        opcode_str = strtok(colon + 1, " \t\n");
    } else {
        opcode_str = strtok(copy, " \t\n");
    }

    /* Read operands */
    operand1 = strtok(NULL, ", \t\n");
    operand2 = strtok(NULL, ", \t\n");

    /* Translate opcode to number */
    if (strcmp(opcode_str, "mov") == 0) opcode = 0;
    else if (strcmp(opcode_str, "cmp") == 0) opcode = 1;
    else if (strcmp(opcode_str, "add") == 0) opcode = 2;
    else if (strcmp(opcode_str, "sub") == 0) opcode = 3;
    else if (strcmp(opcode_str, "not") == 0) opcode = 4;
    else if (strcmp(opcode_str, "clr") == 0) opcode = 5;
    else if (strcmp(opcode_str, "lea") == 0) opcode = 6;
    else if (strcmp(opcode_str, "inc") == 0) opcode = 7;
    else if (strcmp(opcode_str, "dec") == 0) opcode = 8;
    else if (strcmp(opcode_str, "jmp") == 0) opcode = 9;
    else if (strcmp(opcode_str, "bne") == 0) opcode = 10;
    else if (strcmp(opcode_str, "jsr") == 0) opcode = 11;
    else if (strcmp(opcode_str, "red") == 0) opcode = 12;
    else if (strcmp(opcode_str, "prn") == 0) opcode = 13;
    else if (strcmp(opcode_str, "rts") == 0) opcode = 14;
    else if (strcmp(opcode_str, "stop") == 0) opcode = 15;

    /* Set addressing modes */
    if (operand1) {
        src_mode = detect_addressing_mode(operand1);
    }
    if (operand2) {
        dst_mode = detect_addressing_mode(operand2);
    }

    /* Build the instruction word:
       We'll shift bits to encode opcode and modes
       Format:
       Bits 0–3: opcode
       Bits 4–5: src mode
       Bits 6–7: dst mode
    */
    instruction_word |= (opcode & 0xF);          /* 4 bits */
    instruction_word |= ((src_mode & 0x3) << 4); /* 2 bits */
    instruction_word |= ((dst_mode & 0x3) << 6); /* 2 bits */

    return instruction_word;
}


void parse_data_directive(const char *line_ptr) {
    printf("Parsing directive: [%s]\n", line_ptr);

    char buffer[MAX_LINE_LENGTH];
    char *token;
    int i;

    /* נתקדם אחרי .data או .string */
    while (isspace(*line_ptr)) line_ptr++;

    if (strncmp(line_ptr, ".extern", 7) == 0) {
        line_ptr += 7;
        while (isspace(*line_ptr)) line_ptr++;

        /* קרא את שם הסימול החיצוני */
        char label_name[MAX_LINE_LENGTH];
        sscanf(line_ptr, "%s", label_name);

        /* צור סמל חדש */
        Symbol *new_sym = (Symbol *)malloc(sizeof(Symbol));
        if (!new_sym) {
            printf("Memory allocation error in .extern\n");
            return;
        }

        strcpy(new_sym->name, label_name);
        new_sym->address = 0; /* חיצוני – לא ידוע */
        new_sym->is_data = 0;
        new_sym->is_external = 1;
        new_sym->is_entry = 0;
        new_sym->next = symbol_table_head;
        symbol_table_head = new_sym;

        return; /* לא ממשיכים הלאה */
    }

    /* נבדוק איזו הנחיה זו */
    if (strncmp(line_ptr, ".data", 5) == 0) {
        line_ptr += 5;
        while (isspace(*line_ptr)) line_ptr++;

        /* נעתיק את החלק עם המספרים */
        strncpy(buffer, line_ptr, MAX_LINE_LENGTH);
        buffer[MAX_LINE_LENGTH - 1] = '\0';

        token = strtok(buffer, ", \t\n");
        while (token != NULL) {
            int value = atoi(token);

            if (memory_counter < MAX_MEMORY) {
                memory[memory_counter].address = memory_counter;
                memory[memory_counter].value = value;
                memory[memory_counter].is_code = 0; /* זה data */
                memory_counter++;
            } else {
                printf("Error: memory overflow in .data directive.\n");
            }

            token = strtok(NULL, ", \t\n");
        }

    } else if (strncmp(line_ptr, ".string", 7) == 0) {
        line_ptr += 7;
        while (isspace(*line_ptr)) line_ptr++;

        if (*line_ptr != '"') {
            printf("Error: invalid string format.\n");
            return;
        }

        line_ptr++; /* דילוג על גרשיים */

        while (*line_ptr && *line_ptr != '"') {
            if (memory_counter < MAX_MEMORY) {
                memory[memory_counter].address = memory_counter;
                memory[memory_counter].value = (int)(*line_ptr);
                memory[memory_counter].is_code = 0;
                memory_counter++;
                line_ptr++;
            } else {
                printf("Error: memory overflow in .string directive.\n");
                return;
            }
        }

        /* אחרי סוגר " להוסיף null (0) */
        if (memory_counter < MAX_MEMORY) {
            memory[memory_counter].address = memory_counter;
            memory[memory_counter].value = 0;
            memory[memory_counter].is_code = 0;
            memory_counter++;
        }
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
            parse_data_directive(line_ptr);
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

            /* Store this instruction in memory[] as code */
            if (memory_counter < MAX_MEMORY) {
                memory[memory_counter].address = memory_counter;
                memory[memory_counter].value = encode_instruction(line_ptr);
                memory[memory_counter].is_code = 1;
            } else {
                printf("Error: memory overflow when adding instruction at line %d.\n", line_number);
            }

            memory_counter++;  /* Instruction takes 1 memory word */
        }
    }
}

/* Perform the second pass: mark .entry labels, and write .ent, .ext and .ob files */
void second_pass(FILE *fp, const char *orig_filename) {
    rewind(fp);                  /* go back to start of .am file */
    mark_entries(fp);            /* mark entry symbols */
    create_entry_file(orig_filename);
    write_ext_file(orig_filename);
    create_ob_file(orig_filename);
}

/* Goes over the file again and handles .entry lines */
void mark_entries(FILE *fp) {
    char line[MAX_LINE_LENGTH];
    char *line_ptr;
    int line_number = 0;

    rewind(fp);  /* Start reading the file from the beginning */

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        line_number++;

        /* Skip empty lines or comment lines */
        if (line[0] == '\n' || line[0] == ';') {
            continue;
        }

        line_ptr = line;

        /* Skip spaces at the beginning of the line */
        while (isspace(*line_ptr)) {
            line_ptr++;
        }

        /* Check if the line has a .entry directive */
        if (strncmp(line_ptr, ".entry", 6) == 0 || strstr(line_ptr, ".entry") != NULL) {
            char label_name[MAX_LINE_LENGTH];
            Symbol *curr = symbol_table_head;
            bool found = false;

            /* Move pointer past the ".entry" part */
            line_ptr = strstr(line_ptr, ".entry");
            line_ptr += 6;

            /* Skip spaces before the label name */
            while (isspace(*line_ptr)) {
                line_ptr++;
            }

            /* Get the label name */
            sscanf(line_ptr, "%s", label_name);

            /* Go through the symbol table and search for that label */
            while (curr != NULL) {
                if (strcmp(curr->name, label_name) == 0) {
                    curr->is_entry = 1;  /* Mark this symbol as entry */
                    found = true;
                    break;
                }
                curr = curr->next;
            }

            /* If not found, print error message */
            if (!found) {
                printf("Error: entry label '%s' not found in symbol table (line %d).\n", label_name, line_number);
            }
        }
    }
}

/* Creates the .ent file and writes all entry labels */
void create_entry_file(const char *original_filename) {
    FILE *ent_fp;
    char ent_filename[FILENAME_MAX];
    Symbol *curr = symbol_table_head;

    /* Create new filename by replacing .as with .ent */
    strncpy(ent_filename, original_filename, FILENAME_MAX);
    ent_filename[FILENAME_MAX - 1] = '\0';
    char *dot = strrchr(ent_filename, '.');
    if (dot != NULL) {
        strcpy(dot, ".ent");
    } else {
        strcat(ent_filename, ".ent");
    }

    /* Open the file for writing */
    ent_fp = fopen(ent_filename, "w");
    if (!ent_fp) {
        printf("Error: could not create %s file.\n", ent_filename);
        return;
    }

    /* Write all entry symbols to the file */
    while (curr != NULL) {
        if (curr->is_entry) {
            fprintf(ent_fp, "%s %03d\n", curr->name, curr->address);
        }
        curr = curr->next;
    }

    fclose(ent_fp);
}

/* Writes the .ext file that lists where external labels were used */
void write_ext_file(const char *filename) {
    InstructionNode *curr_instr = instruction_head;
    FILE *ext_file;
    char ext_filename[FILENAME_MAX];
    char *opcode, *operand1, *operand2;
    char line_copy[MAX_LINE_LENGTH];
    int addr;

    /* Generate the file name with .ext extension */
    strcpy(ext_filename, filename);
    strcat(ext_filename, ".ext");

    ext_file = fopen(ext_filename, "w");
    if (!ext_file) {
        printf("Error: could not create %s\n", ext_filename);
        return;
    }

    while (curr_instr != NULL) {
        /* Make a copy of the line to tokenize */
        strncpy(line_copy, curr_instr->line, MAX_LINE_LENGTH);
        line_copy[MAX_LINE_LENGTH - 1] = '\0';

        opcode = strtok(line_copy, " \t\n");
        operand1 = strtok(NULL, ", \t\n");
        operand2 = strtok(NULL, ", \t\n");

        addr = curr_instr->address;

        /* Check if operand1 is an external label */
        if (operand1 != NULL) {
            Symbol *sym = symbol_table_head;
            while (sym) {
                if (sym->is_external && strcmp(sym->name, operand1) == 0) {
                    fprintf(ext_file, "%s %d\n", operand1, addr);
                }
                sym = sym->next;
            }
        }

        /* Check if operand2 is an external label */
        if (operand2 != NULL) {
            Symbol *sym = symbol_table_head;
            while (sym) {
                if (sym->is_external && strcmp(sym->name, operand2) == 0) {
                    fprintf(ext_file, "%s %d\n", operand2, addr);
                }
                sym = sym->next;
            }
        }

        curr_instr = curr_instr->next;
    }

    fclose(ext_file);
}

/* Creates the .ob file containing the memory image (code + data) */
void create_ob_file(const char *original_filename) {
    FILE *ob_fp;
    char ob_filename[FILENAME_MAX];
    int i, instruction_count = 0, data_count = 0;

    /* Count instructions and data */
    for (i = 100; i < memory_counter; i++) {
        if (memory[i].is_code)
            instruction_count++;
        else
            data_count++;
    }

    /* Create .ob file name */
    strncpy(ob_filename, original_filename, FILENAME_MAX);
    ob_filename[FILENAME_MAX - 1] = '\0';
    char *dot = strrchr(ob_filename, '.');
    if (dot != NULL) {
        strcpy(dot, ".ob");
    } else {
        strcat(ob_filename, ".ob");
    }

    /* Open file for writing */
    ob_fp = fopen(ob_filename, "w");
    if (!ob_fp) {
        printf("Error: could not create %s file.\n", ob_filename);
        return;
    }

    /* Write header line: number of code and data words */
    fprintf(ob_fp, "%d %d\n", instruction_count, data_count);

    /* Write memory words */
    for (i = 100; i < memory_counter; i++) {
        fprintf(ob_fp, "%03d %d\n", memory[i].address, memory[i].value);
    }

    fclose(ob_fp);
}



void print_memory() {
    int i;

    printf("\n--- Memory Content ---\n");
    for (i = 100; i < memory_counter; i++) {
        printf("Address: %03d | Value: %d | Type: %s\n",
               memory[i].address,
               memory[i].value,
               memory[i].is_code ? "Code" : "Data");
    }
}

void print_symbol_table() {
    printf("\n--- Symbol Table ---\n");
    Symbol *curr = symbol_table_head;
    while (curr) {
        printf("Label: %s | Address: %03d | Type: %s%s%s\n",
               curr->name,
               curr->address,
               curr->is_external ? "External" : (curr->is_data ? "Data" : "Code"),
               curr->is_entry ? " | Entry" : "",
               "");
        curr = curr->next;
    }
}


/* Step 1: Remove extra spaces and tabs, collapse to single spaces */
void remove_extra_spaces_file(const char *in_filename, const char *out_filename) {
    FILE *fin = fopen(in_filename, "r");
    FILE *fout = fopen(out_filename, "w");
    char line[MAX_LINE_LENGTH];
    if (!fin || !fout) {
        fprintf(stderr, "Error: could not open %s or %s for cleaning spaces\n", in_filename, out_filename);
        return;
    }
    while (fgets(line, MAX_LINE_LENGTH, fin)) {
        char buf[MAX_LINE_LENGTH];
        int r = 0, w = 0;
        bool in_space = false;
        /* Trim leading spaces/tabs */
        while (line[r] && isspace((unsigned char)line[r])) r++;
        /* Process rest */
        for (; line[r] && line[r] != '\n'; r++) {
            if (isspace((unsigned char)line[r])) {
                if (!in_space) {
                    buf[w++] = ' ';
                    in_space = true;
                }
            } else {
                buf[w++] = line[r];
                in_space = false;
            }
            if (w >= MAX_LINE_LENGTH - 1) break;
        }
        /* Trim trailing space */
        if (w > 0 && buf[w - 1] == ' ') w--;
        buf[w] = '\0';
        fprintf(fout, "%s\n", buf);
    }
    fclose(fin);
    fclose(fout);
}

int main(int argc, char *argv[]) {
    FILE *fp;
    char t01_filename[FILENAME_MAX];
    char pre_filename[FILENAME_MAX];
    char am_filename[FILENAME_MAX];
    char *dot;

    if (argc < 2) {
        printf("Usage: %s <filename.as>\n", argv[0]);
        return 1;
    }

    /* build .t01 name */
    strncpy(t01_filename, argv[1], FILENAME_MAX);
    t01_filename[FILENAME_MAX - 1] = '\0';
    dot = strrchr(t01_filename, '.');
    if (dot) strcpy(dot, ".t01"); else strcat(t01_filename, ".t01");

    /* build .pre name */
    strncpy(pre_filename, argv[1], FILENAME_MAX);
    pre_filename[FILENAME_MAX - 1] = '\0';
    dot = strrchr(pre_filename, '.');
    if (dot) strcpy(dot, ".pre"); else strcat(pre_filename, ".pre");

    /* build .am name */
    strncpy(am_filename, argv[1], FILENAME_MAX);
    am_filename[FILENAME_MAX - 1] = '\0';
    dot = strrchr(am_filename, '.');
    if (dot) strcpy(dot, ".am"); else strcat(am_filename, ".am");

    /* Step 1: Remove extra spaces → .t01 */
    remove_extra_spaces_file(argv[1], t01_filename);

    /* Step 2: Preprocessor (macros) on cleaned file */
    preprocess_file(t01_filename, pre_filename);

    /* Step 3: Expand macros → .am */
    expand_macros(pre_filename, am_filename);

    /* Step 4: Open .am and First Pass */
    fp = fopen(am_filename, "r");
    if (!fp) {
        perror("Error opening .am file");
        return 1;
    }
    first_pass(fp);

    /* Step 5: Second Pass and outputs */
    rewind(fp);
    mark_entries(fp);
    create_entry_file(argv[1]);
    write_ext_file(argv[1]);
    create_ob_file(argv[1]);

    /* Debug output */
    print_memory();
    print_symbol_table();

    fclose(fp);
    return 0;
}
