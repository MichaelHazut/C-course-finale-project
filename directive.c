#include "directive.h"

/*
 * Skip whitespace and optional label, then test for .data/.string/.extern/.entry
 */
bool is_directive(const char *line) {
    const char *p = line;

    /* skip leading spaces/tabs */
    while (isspace(*p)) p++;

    /* if there is a label, skip past it */
    {
        const char *colon = strchr(p, ':');
        if (colon) {
            p = colon + 1;
            while (isspace(*p)) p++;
        }
    }

    if (strncmp(p, ".data",   5) == 0 ||
        strncmp(p, ".string", 7) == 0 ||
        strncmp(p, ".extern", 7) == 0 ||
        strncmp(p, ".entry",  6) == 0) {
        return true;
    }
    return false;
}

/*
 * Handle one directive line:
 *  - .extern: add a Symbol with is_external=true
 *  - .data:   parse comma-separated ints into memory[]
 *  - .string: read up to closing " and store chars + terminating 0
 */
void parse_data_directive(const char *line_ptr) {
    char buffer[MAX_LINE_LENGTH];
    char *token;
    char label[MAX_LINE_LENGTH];

    /* skip whitespace */
    while (isspace(*line_ptr)) line_ptr++;

    /* extern directive */
    if (strncmp(line_ptr, ".extern", 7) == 0) {
        line_ptr += 7;
        while (isspace(*line_ptr)) line_ptr++;
        sscanf(line_ptr, "%s", label);

        /* add external symbol with address=0 for now */
        add_symbol(label, 0, /*is_data=*/false,
                   /*is_external=*/true, /*is_entry=*/false);
        return;
    }

    /* data directive */
    if (strncmp(line_ptr, ".data", 5) == 0) {
        line_ptr += 5;
        while (isspace(*line_ptr)) line_ptr++;

        strncpy(buffer, line_ptr, MAX_LINE_LENGTH);
        buffer[MAX_LINE_LENGTH - 1] = '\0';

        /* tokenize on commas and whitespace */
        token = strtok(buffer, ", \t\n");
        while (token) {
            int val = atoi(token);

            if (memory_counter < MAX_MEMORY) {
                memory[memory_counter].address = memory_counter;
                memory[memory_counter].value   = val;
                memory[memory_counter].is_code = 0;  /* data */
                memory_counter++;
            }
            token = strtok(NULL, ", \t\n");
        }
        return;
    }

    /* string directive */
    if (strncmp(line_ptr, ".string", 7) == 0) {
        line_ptr += 7;
        while (isspace(*line_ptr)) line_ptr++;

        if (*line_ptr != '"') return;  /* invalid, but skip */

        line_ptr++;  /* skip opening " */
        while (*line_ptr && *line_ptr != '"' && memory_counter < MAX_MEMORY) {
            memory[memory_counter].address = memory_counter;
            memory[memory_counter].value   = (int)(*line_ptr);
            memory[memory_counter].is_code = 0;
            memory_counter++;
            line_ptr++;
        }
        /* terminating null */
        if (memory_counter < MAX_MEMORY) {
            memory[memory_counter].address = memory_counter;
            memory[memory_counter].value   = 0;
            memory[memory_counter].is_code = 0;
            memory_counter++;
        }
        return;
    }

    /* .entry is handled in symbol.mark_entries(), so we ignore it here */
}
