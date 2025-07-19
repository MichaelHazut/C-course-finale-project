#ifndef MACRO_H
#define MACRO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define MAX_LINE_LENGTH 81

typedef struct Macro {
    char name[MAX_LINE_LENGTH];
    char content[MAX_LINE_LENGTH * 10];
    struct Macro *next;
} Macro;

extern Macro *macro_table;

void add_macro(const char *name, const char *content);
void preprocess_file(const char *input_filename, const char *output_filename);
void expand_macros(const char *input_filename, const char *output_filename);

#endif
