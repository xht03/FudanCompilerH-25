.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L118: 
         mov r0, #32
         bl malloc
         mov r5, r0
         mov r0, #7
         str r0, [r5]
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
         add r0, r5, #28
         mov r1, #7
         str r1, [r0]
         mov r0, #8
         bl malloc
         mov r8, r0
         add r0, r8, #0
         mov r1, #2
         str r1, [r0]
         add r0, r8, #4
         ldr r1, =c1$m1
         str r1, [r0]
         mov r0, #8
         bl malloc
         mov r7, r0
         add r0, r7, #0
         mov r1, #2
         str r1, [r0]
         add r0, r7, #4
         ldr r1, =c1$m1
         str r1, [r0]
         ldr r4, [r5]
         mov r6, #0
main$L102: 
         cmp r6, r4
         blt main$L117
main$L103: 
         mov r1, r5
         mov r0, r4
         bl putarray
         mov r0, r4
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L108: 
         mov r0, #-1
         bl exit
main$L109: 
         ldr r2, [r8, #4]
         add r0, r6, #1
         mov r1, #4
         mul r0, r0, r1
         add r0, r5, r0
         mov r1, r6
         mov r0, r8
         blx r2
main$L116: 
         add r6, r6, #1
         b main$L102
main$L112: 
         mov r0, #-1
         bl exit
main$L113: 
         ldr r2, [r7, #4]
         add r0, r6, #1
         mov r1, #4
         mul r0, r0, r1
         add r0, r5, r0
         mov r1, r4
         mov r0, r7
         blx r2
         b main$L116
main$L114: 
         ldr r0, [r5]
         cmp r6, r0
         bge main$L108
         b main$L109
main$L115: 
         ldr r0, [r5]
         cmp r6, r0
         bge main$L112
         b main$L113
main$L117: 
         mov r0, #2
         div r0, r6, r0
         mov r1, #2
         mul r0, r0, r1
         cmp r0, r6
         beq main$L114
         b main$L115

@ Here's function: c1^m1

.balign 4
.global c1$m1
.section .text

c1$m1:
         push {r4-r10, fp, lr}
         add fp, sp, #32
c1$m1$L100: 
         ldr r0, [r0, #0]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}

@ Here's function: c2^m1

.balign 4
.global c2$m1
.section .text

c2$m1:
         push {r4-r10, fp, lr}
         add fp, sp, #32
c2$m1$L100: 
         ldr r0, [r0, #0]
         add r0, r0, r1
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
