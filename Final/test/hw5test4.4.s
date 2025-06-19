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
         mov r0, #20
         bl malloc
         mov r1, #4
         str r1, [r0]
         add r1, r0, #4
         mov r2, #1
         str r2, [r1]
         add r1, r0, #8
         mov r2, #2
         str r2, [r1]
         add r1, r0, #12
         mov r2, #3
         str r2, [r1]
         add r1, r0, #16
         mov r2, #4
         str r2, [r1]
         mov r1, #0
         mov r3, #0
main$L102: 
         mov r2, #4
         cmp r4, r2
         blt main$L108
main$L103: 
         mov r0, r1
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L106: 
         mov r0, #-1
         bl exit
main$L107: 
         add r2, r4, #1
         mov r5, #4
         mul r2, r2, r5
         add r2, r0, r2
         ldr r2, [r2]
         add r1, r1, r2
         add r4, r4, #1
         b main$L102
main$L108: 
         ldr r2, [r0]
         cmp r4, r2
         bge main$L106
         b main$L107

.global malloc
.global getint
.global putint
.global putch
.global putarray
.global getch
.global getarray
.global starttime
.global stoptime
