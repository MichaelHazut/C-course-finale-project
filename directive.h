#ifndef DIRECTIVE_H
#define DIRECTIVE_H

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "symbol.h"
#include "memory.h"

/*
 * Returns true if this line is a directive:
 *   .data, .string, .extern or .entry
 */
bool is_directive(const char *line);

/*
 * Parse one directive line.
 *   - .data   → load integers into memory[]
 *   - .string → load characters (and terminating 0) into memory[]
 *   - .extern → register an external symbol
 */
void parse_data_directive(const char *line);

#endif /* DIRECTIVE_H */
