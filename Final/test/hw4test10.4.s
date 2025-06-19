.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L117: 
         mov r1, #0
         mov r0, r1
main$L102: 
         mov r0, #1
main$L103: 
         mov r1, #1
         add r1, r1, r0
         mov r0, #0
         cmp r1, r0
         bne main$L115
main$L108: 
main$L115: 
main$L116: 
         mov r0, #1
         sub sp, fp, #32
         pop {r4-r10, fp, pc}

.global malloc
.global getint
.global putint
.global putch
.global putarray
.global getch
.global getarray
.global starttime
.global stoptime
