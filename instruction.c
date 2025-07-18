#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "instruction.h"
#include "utils.h"

int is_instruction(const char *line)
{
    const char *p = line;
    /* skip whitespace and labels */
    while (isspace(*p)) p++;
    if (strchr(p, ':')) {
        p = strchr(p, ':') + 1;
        while (isspace(*p)) p++;
    }
    /* compare opcodes */
    if (strncmp(p, "mov", 3)==0 || strncmp(p, "add",3)==0 ||
        strncmp(p, "sub",3)==0 || strncmp(p, "cmp",3)==0 ||
        strncmp(p, "lea",3)==0 || strncmp(p, "clr",3)==0 ||
        strncmp(p, "not",3)==0 || strncmp(p, "inc",3)==0 ||
        strncmp(p, "dec",3)==0 || strncmp(p, "jmp",3)==0 ||
        strncmp(p, "bne",3)==0 || strncmp(p, "jsr",3)==0 ||
        strncmp(p, "red",3)==0 || strncmp(p, "prn",3)==0 ||
        strncmp(p, "rts",3)==0 || strncmp(p, "stop",4)==0) {
        return 1;
    }
    return 0;
}

void parse_instruction(const char *line)
{
    char copy[MAX_LINE_LENGTH], *tok;
    char *opcode, *op1=NULL, *op2=NULL;
    int src_mode, dst_mode;

    strncpy(copy, line, MAX_LINE_LENGTH);
    copy[MAX_LINE_LENGTH-1]='\0';

    /* skip label */
    if ((tok=strchr(copy,':'))) {
        *tok='\0';
        opcode = strtok(copy, " \t\n");
    } else {
        opcode = strtok(copy, " \t\n");
    }

    op1 = strtok(NULL, ", \t\n");
    op2 = strtok(NULL, ", \t\n");

    src_mode = detect_addressing_mode(op1);
    dst_mode = detect_addressing_mode(op2);

    printf("==> Instruction parsed:\n");
    printf("    Opcode: %s\n", opcode);
    if (op1) printf("    Operand 1: %s (mode %d)\n", op1, src_mode);
    if (op2) printf("    Operand 2: %s (mode %d)\n", op2, dst_mode);
}

int encode_instruction(const char *line)
{
    char copy[MAX_LINE_LENGTH], *tok;
    char *opcode_str, *op1=NULL, *op2=NULL;
    int opcode=0, src_mode=0, dst_mode=0, instr=0;

    strncpy(copy, line, MAX_LINE_LENGTH);
    copy[MAX_LINE_LENGTH-1]='\0';

    /* skip label */
    if ((tok=strchr(copy,':'))) {
        opcode_str = strtok(tok+1, " \t\n");
    } else {
        opcode_str = strtok(copy, " \t\n");
    }

    op1 = strtok(NULL, ", \t\n");
    op2 = strtok(NULL, ", \t\n");

    /* opcode→number */
    if      (strcmp(opcode_str,"mov")==0) opcode=0;
    else if (strcmp(opcode_str,"cmp")==0) opcode=1;
    else if (strcmp(opcode_str,"add")==0) opcode=2;
    else if (strcmp(opcode_str,"sub")==0) opcode=3;
    else if (strcmp(opcode_str,"not")==0) opcode=4;
    else if (strcmp(opcode_str,"clr")==0) opcode=5;
    else if (strcmp(opcode_str,"lea")==0) opcode=6;
    else if (strcmp(opcode_str,"inc")==0) opcode=7;
    else if (strcmp(opcode_str,"dec")==0) opcode=8;
    else if (strcmp(opcode_str,"jmp")==0) opcode=9;
    else if (strcmp(opcode_str,"bne")==0) opcode=10;
    else if (strcmp(opcode_str,"jsr")==0) opcode=11;
    else if (strcmp(opcode_str,"red")==0) opcode=12;
    else if (strcmp(opcode_str,"prn")==0) opcode=13;
    else if (strcmp(opcode_str,"rts")==0) opcode=14;
    else if (strcmp(opcode_str,"stop")==0)opcode=15;

    if (op1) src_mode = detect_addressing_mode(op1);
    if (op2) dst_mode = detect_addressing_mode(op2);

    instr |= (opcode     & 0xF);
    instr |= (src_mode   & 0x3) << 4;
    instr |= (dst_mode   & 0x3) << 6;

    return instr;
}
