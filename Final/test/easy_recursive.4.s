.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L100: 
         mov r0, #16
         bl malloc
         mov r4, r0
         add r0, r4, #0
         mov r1, #4
         str r1, [r0]
         add r0, r4, #4
         mov r1, #5
         str r1, [r0]
         add r0, r4, #8
         ldr r1, =c1$m1
         str r1, [r0]
         add r0, r4, #12
         ldr r1, =c2$m2
         str r1, [r0]
         ldr r2, [r4, #12]
         mov r0, r4
         mov r1, #11
         blx r2
         bl putint
         ldr r2, [r4, #12]
         mov r0, r4
         mov r1, #11
         blx r2
         sub sp, fp, #32
         pop {r4-r10, fp, pc}

@ Here's function: c1^m1

.balign 4
.global c1$m1
.section .text

c1$m1:
         push {r4-r10, fp, lr}
         add fp, sp, #32
c1$m1$L100: 
         mov r4, r0
         ldr r3, [r2, #12]
         add r0, r4, #0
         mov r0, r2
         blx r3
         mov r0, r4
         sub sp, fp, #32
         pop {r4-r10, fp, pc}

@ Here's function: c2^m2

.balign 4
.global c2$m2
.section .text

c2$m2:
         push {r4-r10, fp, lr}
         add fp, sp, #32
c2$m2$L105: 
         add r2, r0, #4
         ldr r2, [r0, #0]
         add r2, r2, r1
         mov r2, #256
         cmp r1, r2
         bgt c2$m2$L102
c2$m2$L103: 
c2$m2$L104: 
         ldr r3, [r0, #8]
         add r1, r1, #1
         mov r2, r0
         blx r3
         ldr r0, [r0, #0]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
c2$m2$L102: 
         ldr r1, [r0, #0]
         ldr r0, [r0, #4]
         mul r0, r1, r0
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
