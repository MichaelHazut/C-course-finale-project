macro PRINT_R6
    prn r6
    inc r6
endmacro

MAIN:   mov  r3, LENGTH
        add  r2, r3
        cmp  #-5, LENGTH
        PRINT_R6
        lea  STR, r6
        PRINT_R6
        mov EXT_LABEL, r3
        jmp MYLABEL
        mov  r3, K[ r2 ]
        sub  r1, r4
        bne  END
        stop

STR:    .string "Edge Case!"
LIST:   .data 6, -9, 15, 0, 7
K:      .data 22
LENGTH: .data 4
.entry  STR
.extern EXT_LABEL
.extern MYLABEL

END:    PRINT_R6
