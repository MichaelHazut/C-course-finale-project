#include "utils.h"

int detect_addressing_mode(const char *operand)
{
    if (!operand) return -1;
    if (operand[0] == '#') return 0;
    if (operand[0] == 'r' && isdigit((unsigned char)operand[1]) && operand[2] == '\0')
        return 3;
    if (strchr(operand, '[') && strchr(operand, ']'))
        return 2;
    return 1;
}

int is_label(const char *line)
{
    const char *p = line;
    const char *colon;

    /* skip leading spaces or tabs */
    while (*p == ' ' || *p == '\t') p++;

    colon = strchr(p, ':');
    if (!colon) return 0;

    /* ensure all characters before ':' are alphanumeric */
    for (; p < colon; p++) {
        if (!isalnum((unsigned char)*p)) return 0;
    }
    return 1;
}
