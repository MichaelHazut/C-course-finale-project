#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pass.h"
#include "utils.h"
#include "directive.h"
#include "instruction.h"
#include "symbol.h"
#include "memory.h"

/* First pass: parse each line, register labels, directives, instructions */
void first_pass(FILE *fp)
{
    char line[MAX_LINE_LENGTH];
    char *p;
    int is_lbl, is_dir, is_instr;

    /* rewind before starting */
    rewind(fp);

    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        /* skip blank/comment */
        p = line;
        while (isspace(*p)) p++;
        if (*p == '\0' || *p == ';') continue;

        /* detect label/directive/instruction */
        is_lbl   = is_label(p);
        is_dir   = is_directive(p);
        is_instr = is_instruction(p);

        /* if label, register its address */
        if (is_lbl) {
            char lbl[MAX_LINE_LENGTH];
            sscanf(p, "%[^:]:", lbl);
            add_symbol(lbl, memory_counter,
                       /*is_data=*/is_dir,
                       /*is_external=*/false,
                       /*is_entry=*/false);
            /* advance p past the “label:” */
            p = strchr(p, ':') + 1;
        }

        /* directive? load data or extern */
        if (is_dir) {
            parse_data_directive(p);
        }
        /* instruction? encode and queue */
        else if (is_instr) {
            parse_instruction(p);
            /* encode into memory[] */
            memory[memory_counter].address = memory_counter;
            memory[memory_counter].value   = encode_instruction(p);
            memory[memory_counter].is_code = 1;
            memory_counter++;
        }
    }
}

/* Second pass: mark entries, write output files */
void second_pass(FILE *fp, const char *original_filename)
{
    /* entries */
    mark_entries(fp);

    /* .ent */
    create_entry_file(original_filename);

    /* .ext */
    write_ext_file(original_filename);

    /* .ob */
    create_ob_file(original_filename);
}
