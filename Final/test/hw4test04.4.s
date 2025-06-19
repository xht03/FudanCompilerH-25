.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L109: 
         mov r0, #0
main$L102: 
         mov r0, #1
main$L103: 
         mov r1, #2
         add r0, r1, r0
         mov r1, #0
         cmp r0, r1
         bne main$L106
main$L107: 
         mov r0, #1
main$L108: 
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L106: 
         mov r0, #0
         b main$L108

.global malloc
.global getint
.global putint
.global putch
.global putarray
.global getch
.global getarray
.global starttime
.global stoptime
