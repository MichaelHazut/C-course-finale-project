#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "utils.h"

#define MAX_LINE_LENGTH 81

/* Returns true if this line is a valid instruction */
int is_instruction(const char *line);

/* Parses and prints opcode/operand info (debug) */
void parse_instruction(const char *line);

/* Encodes one instruction into an integer word */
int encode_instruction(const char *line);

#endif /* INSTRUCTION_H */
