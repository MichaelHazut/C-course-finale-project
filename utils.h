#ifndef UTILS_H
#define UTILS_H

#include <ctype.h>
#include <string.h>

/* Detect addressing mode for an operand:
   0 = immediate (#number)
   1 = direct (LABEL)
   2 = index  (LABEL[rX])
   3 = register (rX)
   returns -1 if invalid
*/
int detect_addressing_mode(const char *operand);

/* Returns 1 if the line starts with a valid label (alphanumeric + colon) */
int is_label(const char *line);

#endif /* UTILS_H */