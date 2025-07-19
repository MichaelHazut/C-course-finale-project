#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Boolean type for ANSI C */
#define bool int
#define true 1
#define false 0

/* Maximum line length (for parsing) */
#define MAX_LINE_LENGTH 81

/* Symbol table entry */
typedef struct Symbol {
    char name[MAX_LINE_LENGTH];
    int address;
    bool is_data;
    bool is_external;
    bool is_entry;
    struct Symbol *next;
} Symbol;

/* Head of the symbol table */
extern Symbol *symbol_table_head;

/* Add a symbol to the table */
Symbol *add_symbol(const char *name,
                   int address,
                   bool is_data,
                   bool is_external,
                   bool is_entry);

/* Find a symbol by name */
Symbol *find_symbol(const char *name);

/* Print all symbols */
void print_symbol_table(void);

/* Mark .entry symbols from source file */
void mark_entries(FILE *fp);

/* Create .ent file with entry symbols */
void create_entry_file(const char *original_filename);

void write_ext_file(const char *original_filename);

#endif /* SYMBOL_H */
