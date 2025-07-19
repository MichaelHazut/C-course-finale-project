#include "macro.h"

Macro *macro_table = NULL;

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
            macro_content[0] = '\0';
            continue;
        }

        if (in_macro) {
            if (strncmp(line, "endmacro", 8) == 0) {
                in_macro = 0;
                add_macro(macro_name, macro_content);
                continue;
            }
            strcat(macro_content, "\t");
            strcat(macro_content, line);
        } else {
            fprintf(output_fp, "%s", line);
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

        char *trimmed = line;
        while (isspace(*trimmed)) trimmed++;

        curr_macro = macro_table;
        while (curr_macro != NULL) {
            if (strncmp(trimmed, curr_macro->name, strlen(curr_macro->name)) == 0 &&
                (isspace(trimmed[strlen(curr_macro->name)]) || trimmed[strlen(curr_macro->name)] == '\n')) {
                fprintf(output_fp, "%s", curr_macro->content);
                matched = 1;
                break;
            }
            curr_macro = curr_macro->next;
        }

        if (!matched) {
            fprintf(output_fp, "%s", line);
        }
    }

    fclose(input_fp);
    fclose(output_fp);
}
