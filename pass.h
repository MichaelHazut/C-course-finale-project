#ifndef PASS_H
#define PASS_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "directive.h"
#include "instruction.h"
#include "symbol.h"
#include "memory.h"


/* Perform the first assembler pass:
 *  - build symbol table
 *  - encode instructions/data into memory[]
 */
void first_pass(FILE *fp);

/* Perform the second assembler pass:
 *  - mark .entry symbols
 *  - write .ent, .ext and .ob files
 */
void second_pass(FILE *fp, const char *original_filename);

#endif /* PASS_H */
