#include "symbol.h"

Symbol *symbol_table_head = NULL;

Symbol *add_symbol(const char *name,
                   int address,
                   bool is_data,
                   bool is_external,
                   bool is_entry)
{
    Symbol *new_sym = malloc(sizeof(Symbol));
    if (!new_sym) {
        printf("Error: memory allocation failed in add_symbol.\n");
        return NULL;
    }
    strcpy(new_sym->name, name);
    new_sym->address = address;
    new_sym->is_data = is_data;
    new_sym->is_external = is_external;
    new_sym->is_entry = is_entry;
    new_sym->next = symbol_table_head;
    symbol_table_head = new_sym;
    return new_sym;
}

Symbol *find_symbol(const char *name)
{
    Symbol *curr = symbol_table_head;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void print_symbol_table(void)
{
    printf("\n--- Symbol Table ---\n");
    Symbol *curr = symbol_table_head;
    while (curr) {
        printf("Label: %s | Address: %03d | Type: %s%s\n",
               curr->name,
               curr->address,
               curr->is_external ? "External" : (curr->is_data ? "Data" : "Code"),
               curr->is_entry ? " | Entry" : "");
        curr = curr->next;
    }
}

void mark_entries(FILE *fp)
{
    char line[MAX_LINE_LENGTH];
    char label[MAX_LINE_LENGTH];

    rewind(fp);
    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        if (sscanf(line, ".entry %s", label) == 1) {
            Symbol *sym = find_symbol(label);
            if (sym) {
                sym->is_entry = true;
            } else {
                printf("Warning: entry label '%s' not found.\n", label);
            }
        }
    }
}

void create_entry_file(const char *original_filename)
{
    FILE *ent_fp;
    char ent_filename[FILENAME_MAX];
    Symbol *curr = symbol_table_head;

    /* Build .ent filename from .as */
    strncpy(ent_filename, original_filename, FILENAME_MAX);
    ent_filename[FILENAME_MAX - 1] = '\\0';
    {
        char *dot = strrchr(ent_filename, '.');
        if (dot) strcpy(dot, ".ent");
        else strcat(ent_filename, ".ent");
    }

    ent_fp = fopen(ent_filename, "w");
    if (!ent_fp) {
        printf("Error: could not create %s\\n", ent_filename);
        return;
    }

    while (curr) {
        if (curr->is_entry && !curr->is_external) {
            fprintf(ent_fp, "%s %03d\\n", curr->name, curr->address);
        }
        curr = curr->next;
    }
    fclose(ent_fp);
}

void write_ext_file(const char *original_filename)
{
    FILE *ext_fp;
    char ext_filename[FILENAME_MAX];
    Symbol *curr = symbol_table_head;

    /* build “foo.ext” from “foo.as” */
    strncpy(ext_filename, original_filename, FILENAME_MAX);
    ext_filename[FILENAME_MAX-1] = '\0';
    {
        char *dot = strrchr(ext_filename, '.');
        if (dot) strcpy(dot, ".ext");
        else       strcat(ext_filename, ".ext");
    }

    ext_fp = fopen(ext_filename, "w");
    if (!ext_fp) {
        printf("Error: could not create %s\n", ext_filename);
        return;
    }

    /* list all externals and their addresses */
    while (curr) {
        if (curr->is_external) {
            fprintf(ext_fp, "%s %03d\n", curr->name, curr->address);
        }
        curr = curr->next;
    }
    fclose(ext_fp);
}
