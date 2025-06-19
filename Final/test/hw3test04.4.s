.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L121: 
         mov r0, #44
         bl malloc
         mov r4, r0
         mov r0, #10
         str r0, [r4]
         add r0, r4, #4
         mov r1, #1
         str r1, [r0]
         add r0, r4, #8
         mov r1, #2
         str r1, [r0]
         add r0, r4, #12
         mov r1, #3
         str r1, [r0]
         add r0, r4, #16
         mov r1, #4
         str r1, [r0]
         add r0, r4, #20
         mov r1, #5
         str r1, [r0]
         add r0, r4, #24
         mov r1, #6
         str r1, [r0]
         add r0, r4, #28
         mov r1, #7
         str r1, [r0]
         add r0, r4, #32
         mov r1, #8
         str r1, [r0]
         add r0, r4, #36
         mov r1, #9
         str r1, [r0]
         add r0, r4, #40
         mov r1, #10
         str r1, [r0]
         mov r0, #16
         bl malloc
         mov r1, #3
         str r1, [r0]
         add r1, r0, #4
         mov r2, #3
         str r2, [r1]
         add r1, r0, #8
         mov r2, #4
         str r2, [r1]
         add r1, r0, #12
         mov r2, #5
         str r2, [r1]
         ldr r1, [r0]
         mov r2, #0
         cmp r2, r1
         bge main$L102
main$L103: 
         ldr r2, [r0, #4]
         ldr r1, [r4]
         cmp r2, r1
         bge main$L106
main$L107: 
         add r1, r2, #1
         mov r2, #4
         mul r1, r1, r2
         add r1, r4, r1
         ldr r1, [r1]
         mov r2, #1
         cmp r1, r2
         blt main$L110
main$L111: 
main$L112: 
main$L116: 
         ldr r1, [r0]
         mov r2, #0
         cmp r2, r1
         bge main$L119
main$L120: 
         ldr r0, [r0, #4]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L102: 
         mov r0, #-1
         bl exit
main$L106: 
         mov r0, #-1
         bl exit
main$L110: 
         mov r0, r4
         b main$L112
main$L119: 
         mov r0, #-1
         bl exit

.global malloc
.global getint
.global putint
.global putch
.global putarray
.global getch
.global getarray
.global starttime
.global stoptime
