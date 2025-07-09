#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 81

/* Define boolean type for ANSI C */
#define bool int
#define true 1
#define false 0

/* Checks if a line starts with a label (ends with a colon) */
bool is_label(const char *line) {
    const char *colon;
    const char *p;

    /* Skip leading spaces or tabs */
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    /* Look for a colon (':') which indicates a label */
    colon = strchr(line, ':');
    if (!colon) {
        return false;
    }

    /* Check that all characters before the ':' are alphanumeric */
    for (p = line; p < colon; p++) {
        if (!isalnum(*p)) {
            return false;
        }
    }

    return true;
}


/* Checks if the line contains a directive like .data or .string */
bool is_directive(const char *line) {
    const char *p;

    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    /* If there's a label, skip past it */
    p = strchr(line, ':');
    if (p) {
        line = p + 1;
        while (*line == ' ' || *line == '\t') {
            line++;
        }
    }

    /* Check for known directives */
    if (strncmp(line, ".data", 5) == 0 ||
        strncmp(line, ".string", 7) == 0 ||
        strncmp(line, ".extern", 7) == 0 ||
        strncmp(line, ".entry", 6) == 0) {
        return true;
        }

    return false;
}


int main(int argc, char *argv[]) {
    FILE *fp;
    char line[MAX_LINE_LENGTH];

    /* If the user has not given a file name */
    if (argc < 2) {
        printf("Usage: %s <filename.as>\n", argv[0]);
        return 1;
    }

    /* Open the file in read mode */
    fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    /* First Pass: Print full contents of the file */
    printf("\n1) Full contents of the file:\n");

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        printf("%s", line);
    }

    /* Second Pass: Analyze each line */
    rewind(fp);
    printf("\n\n\n2) Line analysis:");

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        printf("\n>> Line: | %s", line);

        bool has_label = is_label(line);
        bool has_directive = is_directive(line);

        if (has_label) {
            printf("=> Type: Label\n");
        }
        if (has_directive) {
            printf("=> Type: Directive\n");
        }
        if (!has_label && !has_directive) {
            printf("=> Type: Not a label or directive\n");
        }

    }

    fclose(fp);
    return 0;
}
