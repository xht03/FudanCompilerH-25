.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L108: 
         mov r0, #44
         bl malloc
         mov r1, #10
         str r1, [r0]
         ldr r1, [r0]
         mov r2, #0
         cmp r2, r1
         bge main$L102
main$L103: 
         add r1, r0, #4
         mov r2, #1
         str r2, [r1]
         ldr r1, [r0]
         mov r2, #0
         cmp r2, r1
         bge main$L106
main$L107: 
         ldr r0, [r0, #4]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L102: 
         mov r0, #-1
         bl exit
main$L106: 
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
