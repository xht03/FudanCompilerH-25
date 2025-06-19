.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L114: 
main$L100: 
main$L106: 
         mov r0, #0
main$L104: 
         mov r0, #1
main$L105: 
         mov r1, #2
         add r0, r1, r0
main$L101: 
         mov r1, #0
         cmp r0, r1
         bne main$L111
main$L112: 
         mov r0, #1
main$L113: 
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L111: 
         mov r0, #0
         b main$L113

.global malloc
.global getint
.global putint
.global putch
.global putarray
.global getch
.global getarray
.global starttime
.global stoptime
