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
         mov r0, #44
         bl malloc
         mov r5, r0
         mov r1, #10
         str r1, [r5]
         add r0, r5, #4
         mov r1, #1
         str r1, [r0]
         add r0, r5, #8
         mov r1, #2
         str r1, [r0]
         add r0, r5, #12
         mov r1, #3
         str r1, [r0]
         add r0, r5, #16
         mov r1, #4
         str r1, [r0]
         add r0, r5, #20
         mov r1, #5
         str r1, [r0]
         add r0, r5, #24
         mov r1, #6
         str r1, [r0]
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
         ldr r6, [r5]
         ldr r0, [r4]
         cmp r6, r0
         bne main$L102
main$L103: 
         add r0, r6, #1
         mov r1, #4
         mul r0, r0, r1
         bl malloc
         str r6, [r0]
         add r1, r6, #1
         mov r2, #4
         mul r3, r1, r2
         mov r1, #4
main$L108: 
         cmp r1, r3
         blt main$L107
main$L106: 
         ldr r1, [r0]
         mov r2, #1
         cmp r2, r1
         bge main$L111
main$L112: 
         ldr r1, [r0, #8]
         ldr r0, [r4]
         mov r2, #0
         cmp r2, r0
         bge main$L115
main$L116: 
         ldr r0, [r4, #4]
         add r0, r1, r0
         bl putint
         mov r0, #10
         bl putch
         mov r0, #0
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L102: 
         mov r0, #-1
         bl exit
main$L107: 
         add r2, r0, r1
         add r2, r5, r1
         ldr r6, [r2]
         add r2, r4, r1
         ldr r2, [r2]
         add r2, r6, r2
         add r1, r1, #4
         b main$L108
main$L111: 
         mov r0, #-1
         bl exit
main$L115: 
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
