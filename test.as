MAIN:    mov  r3, r1
         add  r1, LENGTH
         sub  r2, r5
         stop

LENGTH:  .data  6, -9, 15

STR:     .string "Hello!"

         .extern  EXT1

LOOP:    cmp  r3, #0
         bne  LOOP

         .entry MAIN
         .entry LENGTH
