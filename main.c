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

/* bit-masks for addressing modes */
#define MODE_IMM (1<<0)
#define MODE_DIR (1<<1)
#define MODE_IDX (1<<2)
#define MODE_REG (1<<3)

/* Information for each opcode */
typedef struct {
    const char *name;
    int code;
    int num_operands;
    int src_mask;   /* which modes allowed for src */
    int dst_mask;   /* which modes allowed for dst */
} OpcodeInfo;

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

/* static table of all 16 opcodes */
static const OpcodeInfo opcode_table[] = {
    {"mov", 0, 2, MODE_IMM|MODE_DIR|MODE_IDX|MODE_REG, MODE_DIR|MODE_IDX|MODE_REG},
    {"cmp", 1, 2, MODE_IMM|MODE_DIR|MODE_IDX|MODE_REG, MODE_IMM|MODE_DIR|MODE_IDX|MODE_REG},
    {"add", 2, 2, MODE_IMM|MODE_DIR|MODE_IDX|MODE_REG, MODE_DIR|MODE_IDX|MODE_REG},
    {"sub", 3, 2, MODE_IMM|MODE_DIR|MODE_IDX|MODE_REG, MODE_DIR|MODE_IDX|MODE_REG},
    {"not", 4, 1, 0,                        MODE_DIR|MODE_IDX|MODE_REG},
    {"clr", 5, 1, 0,                        MODE_DIR|MODE_IDX|MODE_REG},
    {"lea", 6, 2, MODE_DIR|MODE_IDX,       MODE_DIR|MODE_IDX|MODE_REG},
    {"inc", 7, 1, 0,                        MODE_DIR|MODE_IDX|MODE_REG},
    {"dec", 8, 1, 0,                        MODE_DIR|MODE_IDX|MODE_REG},
    {"jmp", 9, 1, 0,                        MODE_DIR|MODE_IDX|MODE_REG},
    {"bne",10, 1, 0,                        MODE_DIR|MODE_IDX|MODE_REG},
    {"jsr",11, 1, 0,                        MODE_DIR|MODE_IDX|MODE_REG},
    {"red",12, 1, 0,                        MODE_DIR|MODE_IDX|MODE_REG},
    {"prn",13, 1, MODE_IMM|MODE_DIR|MODE_IDX|MODE_REG, 0},
    {"rts",14, 0, 0,                        0},
    {"stop",15,0, 0,                        0}
};
static const int OPCODE_COUNT = sizeof(opcode_table)/sizeof(opcode_table[0]);

/* Lookup an OpcodeInfo by name, or return NULL */
const OpcodeInfo* find_opcode(const char *name) {
    int i;
    for (i = 0; i < OPCODE_COUNT; i++) {
        if (strcmp(opcode_table[i].name, name) == 0)
            return &opcode_table[i];
    }
    return NULL;
}


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

/* Implementation */
Symbol *find_symbol(const char *name) {
    Symbol *cur = symbol_table_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

/* Validate operand count and addressing modes for one instruction */
bool validate_instruction(const char *line, int line_num, const char *file_name) {
    char buf[MAX_LINE_LENGTH];
    strncpy(buf, line, MAX_LINE_LENGTH);
    buf[MAX_LINE_LENGTH-1] = '\0';

    /* strip label */
    char *instr = strchr(buf, ':');
    if (instr) instr++;
    else instr = buf;

    /* parse opcode and operands */
    char *opc = strtok(instr, " \t\n");
    if (!opc) return true;  /* nothing to do */

    const OpcodeInfo *info = find_opcode(opc);
    if (!info) {
        fprintf(stderr, "%s:%d: error: unknown opcode '%s'\n", file_name, line_num, opc);
        return false;
    }

    /* count operands */
    char *op1 = strtok(NULL, ", \t\n");
    char *op2 = strtok(NULL, ", \t\n");
    int count = (op2 ? 2 : (op1 ? 1 : 0));
    if (count != info->num_operands) {
        fprintf(stderr, "%s:%d: error: '%s' expects %d operands, got %d\n",
                file_name, line_num, opc, info->num_operands, count);
        return false;
    }

    /* check addressing modes */
    if (op1) {
        int m1 = detect_addressing_mode(op1);
        if (!(info->src_mask & (1<<m1))) {
            fprintf(stderr, "%s:%d: error: addressing mode %d not allowed for source of '%s'\n",
                    file_name, line_num, m1, opc);
            return false;
        }
    }
    if (op2) {
        int m2 = detect_addressing_mode(op2);
        if (!(info->dst_mask & (1<<m2))) {
            fprintf(stderr, "%s:%d: error: addressing mode %d not allowed for dest of '%s'\n",
                    file_name, line_num, m2, opc);
            return false;
        }
    }
    return true;
}


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

bool macro_exists(const char *name) {
    Macro *cur = macro_table;
    while (cur) {
        if (strcmp(cur->name, name) == 0) return true;
        cur = cur->next;
    }
    return false;
}


void preprocess_file(const char *input_filename, const char *output_filename) {
    FILE *input_fp = fopen(input_filename, "r");
    FILE *output_fp = fopen(output_filename, "w");
    char line[MAX_LINE_LENGTH];
    char macro_name[MAX_LINE_LENGTH];
    char macro_content[MAX_LINE_LENGTH * 10];
    bool in_macro = false;

    if (!input_fp || !output_fp) {
        fprintf(stderr, "Error: failed to open %s or %s\n", input_filename, output_filename);
        return;
    }

    while (fgets(line, MAX_LINE_LENGTH, input_fp)) {
        /* התחלת הגדרת מאקרו */
        if (strncmp(line, "macro", 5) == 0) {
            if (in_macro) {
                fprintf(stderr, "%s: error: nested macro definitions not allowed\n", input_filename);
                return;
            }
            sscanf(line, "macro %s", macro_name);
            if (macro_exists(macro_name)) {
                fprintf(stderr, "%s: error: duplicate macro '%s'\n", input_filename, macro_name);
                return;
            }
            in_macro = true;
            macro_content[0] = '\0';
            continue;
        }

        /* סוף הגדרת מאקרו */
        if (in_macro && strncmp(line, "endmacro", 8) == 0) {
            in_macro = false;
            add_macro(macro_name, macro_content);
            continue;
        }

        if (in_macro) {
            /* בתוך מאקרו – מצטבר לתוכן */
            strcat(macro_content, line);
        } else {
            /* מחוץ למאקרו – כותב לשורה ב־.pre */
            fprintf(output_fp, "%s", line);
        }
    }

    if (in_macro) {
        fprintf(stderr, "%s: error: missing endmacro for '%s'\n", input_filename, macro_name);
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
    char buffer[MAX_LINE_LENGTH];
    char *token;

    /* Skip leading whitespace */
    while (isspace((unsigned char)*line_ptr)) line_ptr++;

    /* .extern directive */
    if (strncmp(line_ptr, ".extern", 7) == 0) {
        line_ptr += 7;
        while (isspace((unsigned char)*line_ptr)) line_ptr++;

        char label_name[MAX_LINE_LENGTH];
        if (sscanf(line_ptr, "%s", label_name) != 1) {
            fprintf(stderr, "Error: invalid .extern syntax\n");
            return;
        }

        Symbol *new_sym = malloc(sizeof(Symbol));
        if (!new_sym) {
            fprintf(stderr, "Error: memory allocation failed in .extern\n");
            return;
        }
        strcpy(new_sym->name, label_name);
        new_sym->address     = 0;
        new_sym->is_data     = false;
        new_sym->is_external = true;
        new_sym->is_entry    = false;
        new_sym->next        = symbol_table_head;
        symbol_table_head    = new_sym;
        return;
    }

    /* .data directive */
    if (strncmp(line_ptr, ".data", 5) == 0) {
        line_ptr += 5;
        while (isspace((unsigned char)*line_ptr)) line_ptr++;

        strncpy(buffer, line_ptr, MAX_LINE_LENGTH);
        buffer[MAX_LINE_LENGTH-1] = '\0';

        token = strtok(buffer, ", \t\n");
        while (token) {
            int value = atoi(token);
            if (memory_counter >= MAX_MEMORY) {
                fprintf(stderr, "Error: memory overflow in .data\n");
                return;
            }
            memory[memory_counter].address = memory_counter;
            memory[memory_counter].value   = value;
            memory[memory_counter].is_code = 0;
            memory_counter++;
            token = strtok(NULL, ", \t\n");
        }
        return;
    }

    /* .string directive */
    if (strncmp(line_ptr, ".string", 7) == 0) {
        line_ptr += 7;
        while (isspace((unsigned char)*line_ptr)) line_ptr++;

        if (*line_ptr != '"') {
            fprintf(stderr, "Error: invalid .string format\n");
            return;
        }
        line_ptr++;  /* skip opening quote */

        while (*line_ptr && *line_ptr != '"') {
            if (memory_counter >= MAX_MEMORY) {
                fprintf(stderr, "Error: memory overflow in .string\n");
                return;
            }
            memory[memory_counter].address = memory_counter;
            memory[memory_counter].value   = (int)*line_ptr;
            memory[memory_counter].is_code = 0;
            memory_counter++;
            line_ptr++;
        }
        if (*line_ptr != '"') {
            fprintf(stderr, "Error: missing closing quote in .string\n");
            return;
        }
        /* add null terminator */
        if (memory_counter >= MAX_MEMORY) {
            fprintf(stderr, "Error: memory overflow in .string\n");
            return;
        }
        memory[memory_counter].address = memory_counter;
        memory[memory_counter].value   = 0;
        memory[memory_counter].is_code = 0;
        memory_counter++;
        return;
    }

    /* .mat directive */
    if (strncmp(line_ptr, ".mat", 4) == 0) {
        int rows, cols;
        /* parse dimensions */
        if (sscanf(line_ptr + 4, " [%d][%d]", &rows, &cols) != 2 || rows <= 0 || cols <= 0) {
            fprintf(stderr, "Error: invalid .mat dimensions\n");
            return;
        }
        /* move past the closing ']' */
        char *p = strchr(line_ptr, ']');
        if (!p || !(p = strchr(p + 1, ']'))) {
            fprintf(stderr, "Error: malformed .mat directive\n");
            return;
        }
        p++;  /* now at initializer list or end */

        while (isspace((unsigned char)*p)) p++;

        int total = rows * cols;
        int init_count = 0;

        if (*p && *p != '\n') {
            strncpy(buffer, p, MAX_LINE_LENGTH);
            buffer[MAX_LINE_LENGTH-1] = '\0';
            token = strtok(buffer, ", \t\n");
            while (token && init_count < total) {
                int value = atoi(token);
                if (memory_counter >= MAX_MEMORY) {
                    fprintf(stderr, "Error: memory overflow in .mat\n");
                    return;
                }
                memory[memory_counter].address = memory_counter;
                memory[memory_counter].value   = value;
                memory[memory_counter].is_code = 0;
                memory_counter++;
                init_count++;
                token = strtok(NULL, ", \t\n");
            }
            if (token) {
                fprintf(stderr, "Error: too many initializers for .mat\n");
                return;
            }
        }
        /* zero-fill remaining */
        while (init_count < total) {
            if (memory_counter >= MAX_MEMORY) {
                fprintf(stderr, "Error: memory overflow in .mat\n");
                return;
            }
            memory[memory_counter].address = memory_counter;
            memory[memory_counter].value   = 0;
            memory[memory_counter].is_code = 0;
            memory_counter++;
            init_count++;
        }
        return;
    }

    /* unknown directive */
    fprintf(stderr, "Error: unrecognized directive in parse_data_directive\n");
}


void first_pass(FILE *fp, const char *filename) {
    char line[MAX_LINE_LENGTH];
    char *line_ptr;
    int line_number = 0;

    rewind(fp);

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        Symbol *new_sym;
        InstructionNode *new_instr;
        char label_name[MAX_LINE_LENGTH];
        int is_label_line, is_dir, is_instr;

        line_number++;

        /* Skip empty or comment lines */
        if (line[0] == '\n' || line[0] == ';') {
            continue;
        }

        /* Trim leading whitespace */
        line_ptr = line;
        while (isspace((unsigned char)*line_ptr)) {
            line_ptr++;
        }

        is_label_line = is_label(line_ptr);
        is_dir        = is_directive(line_ptr);
        is_instr      = is_instruction(line_ptr);

        /* Handle label definition */
        if (is_label_line) {
            /* extract label name */
            sscanf(line_ptr, "%[^:]:", label_name);

            new_sym = malloc(sizeof(Symbol));
            if (!new_sym) {
                fprintf(stderr, "%s:%d: error: memory allocation failed for symbol\n", filename, line_number);
                continue;
            }
            strcpy(new_sym->name, label_name);
            new_sym->address      = memory_counter;
            new_sym->is_entry     = false;
            new_sym->is_external  = false;
            new_sym->is_data      = is_dir;
            new_sym->next         = symbol_table_head;
            symbol_table_head     = new_sym;

            /* advance pointer past label */
            line_ptr = strchr(line_ptr, ':');
            if (line_ptr) {
                line_ptr++;
                while (isspace((unsigned char)*line_ptr)) {
                    line_ptr++;
                }
            }
        }

        /* Handle directive (.data/.string/.extern/.entry/.mat) */
        if (is_dir) {
            parse_data_directive(line_ptr);
        }
        /* Handle instruction */
        else if (is_instr) {
            /* validate operand count and addressing modes */
            if (!validate_instruction(line_ptr, line_number, filename)) {
                continue;
            }

            /* create new instruction node */
            new_instr = malloc(sizeof(InstructionNode));
            if (!new_instr) {
                fprintf(stderr, "%s:%d: error: memory allocation failed for instruction node\n", filename, line_number);
                continue;
            }
            new_instr->address = memory_counter;
            strncpy(new_instr->line, line_ptr, MAX_LINE_LENGTH);
            new_instr->line[MAX_LINE_LENGTH - 1] = '\0';
            new_instr->next = NULL;

            /* append to instruction list */
            if (instruction_head == NULL) {
                instruction_head = new_instr;
                instruction_tail = new_instr;
            } else {
                instruction_tail->next = new_instr;
                instruction_tail = new_instr;
            }

            /* encode primary instruction word */
            if (memory_counter < MAX_MEMORY) {
                memory[memory_counter].address = memory_counter;
                memory[memory_counter].value   = encode_instruction(line_ptr);
                memory[memory_counter].is_code = 1;
            } else {
                fprintf(stderr, "%s:%d: error: memory overflow when adding instruction\n", filename, line_number);
            }
            memory_counter++;
        }
        /* Unknown or invalid line */
        else {
            fprintf(stderr, "%s:%d: error: unrecognized statement\n", filename, line_number);
        }
    }
}

void generate_extra_operand_words(void) {
    InstructionNode *cur;
    Symbol *sym;
    MemoryWord temp[MAX_MEMORY];
    int extra, i, k, new_cnt;
    char buf1[MAX_LINE_LENGTH];
    char buf2[MAX_LINE_LENGTH];
    char *tok1, *tok2;
    char *ops1[2];
    char *ops2[2];
    int m1, m2, modes[2], val;

    /* 1) Count how many extra words are needed */
    extra = 0;
    cur = instruction_head;
    while (cur) {
        /* tokenize operands */
        strncpy(buf1, cur->line, MAX_LINE_LENGTH);
        buf1[MAX_LINE_LENGTH-1] = '\0';
        tok1 = strtok(buf1, " \t\n");      /* skip opcode */
        ops1[0] = strtok(NULL, ", \t\n");
        ops1[1] = strtok(NULL, ", \t\n");

        m1 = ops1[0] ? detect_addressing_mode(ops1[0]) : -1;
        m2 = ops1[1] ? detect_addressing_mode(ops1[1]) : -1;
        if (m1 == 3 && m2 == 3) {
            extra += 1;
        } else {
            if (m1 >= 0) extra++;
            if (m2 >= 0) extra++;
        }
        cur = cur->next;
    }

    /* 2) Shift data symbol addresses */
    for (sym = symbol_table_head; sym; sym = sym->next) {
        if (sym->is_data) {
            sym->address += extra;
        }
    }
    /* Shift existing data words in memory[] */
    for (i = 100; i < memory_counter; i++) {
        if (!memory[i].is_code) {
            memory[i].address += extra;
        }
    }

    /* 3) Backup old memory */
    memcpy(temp, memory, sizeof(memory));

    /* 4) Rebuild code words with extra operand words */
    new_cnt = 100;
    cur     = instruction_head;
    while (cur) {
        /* copy primary instruction word */
        memory[new_cnt]         = temp[cur->address];
        memory[new_cnt].address = new_cnt;
        new_cnt++;

        /* parse operands again */
        strncpy(buf2, cur->line, MAX_LINE_LENGTH);
        buf2[MAX_LINE_LENGTH-1] = '\0';
        tok2     = strtok(buf2, " \t\n");  /* skip opcode */
        ops2[0]  = strtok(NULL, ", \t\n");
        ops2[1]  = strtok(NULL, ", \t\n");
        modes[0] = ops2[0] ? detect_addressing_mode(ops2[0]) : -1;
        modes[1] = ops2[1] ? detect_addressing_mode(ops2[1]) : -1;

        /* if both operands are registers, pack into one word */
        if (modes[0] == 3 && modes[1] == 3) {
            int r1 = ops2[0][1] - '0';
            int r2 = ops2[1][1] - '0';
            memory[new_cnt].address = new_cnt;
            memory[new_cnt].value   = (r1 << 4) | r2;
            memory[new_cnt].is_code = 1;
            new_cnt++;
        } else {
            /* otherwise generate one word per operand */
            for (k = 0; k < 2; k++) {
                if (modes[k] < 0) continue;
                if (modes[k] == 0) {           /* immediate */
                    val = atoi(ops2[k] + 1);
                } else if (modes[k] == 1) {    /* direct */
                    sym = find_symbol(ops2[k]);
                    val = sym ? sym->address : 0;
                } else if (modes[k] == 2) {    /* index */
                    char lbl[MAX_LINE_LENGTH];
                    int reg;
                    sscanf(ops2[k], "%[^[][%*c%d%*c]", lbl, &reg);
                    sym = find_symbol(lbl);
                    /* write base address word */
                    memory[new_cnt].address = new_cnt;
                    memory[new_cnt].value   = sym ? sym->address : 0;
                    memory[new_cnt].is_code = 1;
                    new_cnt++;
                    val = reg;
                } else {                       /* single register */
                    val = ops2[k][1] - '0';
                }
                memory[new_cnt].address = new_cnt;
                memory[new_cnt].value   = val;
                memory[new_cnt].is_code = 1;
                new_cnt++;
            }
        }
        cur = cur->next;
    }

    /* 5) Append data words after code */
    for (i = 100; i < memory_counter; i++) {
        if (!temp[i].is_code) {
            memory[new_cnt]         = temp[i];
            memory[new_cnt].address = new_cnt;
            new_cnt++;
        }
    }

    /* 6) Update counter */
    memory_counter = new_cnt;
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

/* Convert a 12-bit word into 5 “base-4” chars: a=00, b=01, c=10, d=11 */
void word_to_base4(int word, char out[6]) {
    int i;
    /* We only encode the lower 12 bits: bits 0–11 */
    /* We split into 5 groups of 2 bits each: group 0 = bits 8–9, ..., group 4 = bits 0–1 */
    for (i = 0; i < 5; i++) {
        int shift = (4 - i) * 2;
        int two_bits = (word >> shift) & 0x3; /* extract 2 bits */
        /* map 0→'a', 1→'b', 2→'c', 3→'d' */
        out[i] = (char)('a' + two_bits);
    }
    out[5] = '\0';
}


/* Creates the .ob file containing the memory image (code + data) */
void create_ob_file(const char *original_filename) {
    FILE *ob_fp;
    char ob_filename[FILENAME_MAX];
    int i, instruction_count = 0, data_count = 0;
    char base4[6];

    /* Count instructions and data */
    for (i = 100; i < memory_counter; i++) {
        if (memory[i].is_code)
            instruction_count++;
        else
            data_count++;
    }

    /* Construct .ob filename */
    strncpy(ob_filename, original_filename, FILENAME_MAX);
    ob_filename[FILENAME_MAX - 1] = '\0';
    {
        char *dot = strrchr(ob_filename, '.');
        if (dot) strcpy(dot, ".ob");
        else    strcat(ob_filename, ".ob");
    }

    ob_fp = fopen(ob_filename, "w");
    if (!ob_fp) {
        fprintf(stderr, "Error: could not create %s file.\n", ob_filename);
        return;
    }

    /* Header: code words count and data words count */
    fprintf(ob_fp, "%d %d\n", instruction_count, data_count);

    /* Write each memory word in base-4 encoding */
    for (i = 100; i < memory_counter; i++) {
        word_to_base4(memory[i].value, base4);
        fprintf(ob_fp, "%03d %s\n", memory[i].address, base4);
    }

    fclose(ob_fp);
}


/* Perform the second pass: mark .entry labels, and write .ent, .ext and .ob files */
void second_pass(FILE *fp, const char *orig_filename) {
    rewind(fp);
    mark_entries(fp);
    generate_extra_operand_words();
    create_entry_file(orig_filename);
    write_ext_file(orig_filename);
    create_ob_file(orig_filename);
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

/* Remove macro definitions (macro ... endmacro) -> .t02 */
void remove_macro_decls_file(const char *in_filename, const char *out_filename) {
    FILE *fin = fopen(in_filename, "r");
    FILE *fout = fopen(out_filename, "w");
    char line[MAX_LINE_LENGTH];
    bool in_macro = false;

    if (!fin || !fout) {
        fprintf(stderr, "Error: cannot open %s or %s for macro-stripping\n", in_filename, out_filename);
        return;
    }

    while (fgets(line, MAX_LINE_LENGTH, fin)) {
        /* start of macro definition? */
        if (!in_macro && strncmp(line, "macro", 5) == 0) {
            in_macro = true;
            continue;
        }
        /* end of macro definition? */
        if (in_macro && strncmp(line, "endmacro", 8) == 0) {
            in_macro = false;
            continue;
        }
        /* if not inside a macro definition, write the line */
        if (!in_macro) {
            fprintf(fout, "%s", line);
        }
    }

    fclose(fin);
    fclose(fout);
}

/* Remove spaces immediately before or after commas -> .t01a */
void remove_spaces_next_to_comma_file(const char *in_filename, const char *out_filename) {
    FILE *fin = fopen(in_filename, "r");
    FILE *fout = fopen(out_filename, "w");
    char line[MAX_LINE_LENGTH];
    if (!fin || !fout) {
        fprintf(stderr, "Error: cannot open %s or %s for comma-spacing\n", in_filename, out_filename);
        return;
    }
    while (fgets(line, MAX_LINE_LENGTH, fin)) {
        char buf[MAX_LINE_LENGTH];
        int r = 0, w = 0;
        char c;
        while ((c = line[r++]) && c != '\n') {
            if (c == ' ' && line[r] == ',') continue;
            if (c == ',' && line[r] == ' ') {
                buf[w++] = ',';
                r++;
                continue;
            }
            buf[w++] = c;
        }
        buf[w] = '\0';
        fprintf(fout, "%s\n", buf);
    }
    fclose(fin);
    fclose(fout);
}


int main(int argc, char *argv[]) {
    FILE *fp;
    char t01_filename[FILENAME_MAX];
    char t01a_filename[FILENAME_MAX];  /* after comma-cleaning */
    char t02_filename[FILENAME_MAX];
    char pre_filename[FILENAME_MAX];
    char am_filename[FILENAME_MAX];
    char *dot;

    if (argc < 2) {
        printf("Usage: %s <filename.as>\n", argv[0]);
        return 1;
    }

    /* build .t01 name */
    strncpy(t01_filename, argv[1], FILENAME_MAX);
    t01_filename[FILENAME_MAX-1] = '\0';
    dot = strrchr(t01_filename, '.');
    if (dot) strcpy(dot, ".t01"); else strcat(t01_filename, ".t01");

    /* build .t01a name */
    strncpy(t01a_filename, t01_filename, FILENAME_MAX);
    t01a_filename[FILENAME_MAX-1] = '\0';
    dot = strrchr(t01a_filename, '.');
    if (dot) strcpy(dot, ".t01a"); else strcat(t01a_filename, ".t01a");

    /* build .t02 name */
    strncpy(t02_filename, t01a_filename, FILENAME_MAX);
    t02_filename[FILENAME_MAX-1] = '\0';
    dot = strrchr(t02_filename, '.');
    if (dot) strcpy(dot, ".t02"); else strcat(t02_filename, ".t02");

    /* build .pre name */
    strncpy(pre_filename, argv[1], FILENAME_MAX);
    pre_filename[FILENAME_MAX-1] = '\0';
    dot = strrchr(pre_filename, '.');
    if (dot) strcpy(dot, ".pre"); else strcat(pre_filename, ".pre");

    /* build .am name */
    strncpy(am_filename, argv[1], FILENAME_MAX);
    am_filename[FILENAME_MAX-1] = '\0';
    dot = strrchr(am_filename, '.');
    if (dot) strcpy(dot, ".am"); else strcat(am_filename, ".am");

    /* Step 1: Remove extra spaces -> .t01 */
    remove_extra_spaces_file(argv[1], t01_filename);

    /* Step 1b: Remove spaces around commas -> .t01a */
    remove_spaces_next_to_comma_file(t01_filename, t01a_filename);

    /* Step 2: Strip out macro definitions -> .t02 */
    remove_macro_decls_file(t01a_filename, t02_filename);

    /* Step 3: Preprocessor (collect definitions) -> .pre */
    preprocess_file(t02_filename, pre_filename);

    /* Step 4: Expand macros -> .am */
    expand_macros(pre_filename, am_filename);

    /* Step 5: First Pass */
    fp = fopen(am_filename, "r");
    if (!fp) {
        perror("Error opening .am file");
        return 1;
    }
    first_pass(fp, argv[1]);

    /* Step 6: Second Pass and output files */
    rewind(fp);
    mark_entries(fp);
    create_entry_file(argv[1]);
    write_ext_file(argv[1]);
    create_ob_file(argv[1]);

    /* Debug prints */
    print_memory();
    print_symbol_table();

    fclose(fp);
    return 0;
}
