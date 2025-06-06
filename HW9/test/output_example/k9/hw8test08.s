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
         mov r0, #4
         bl malloc
         mov r5, r0
         add r0, r5, #0
         ldr r1, =C$m
         str r1, [r0]
         mov r0, #4
         bl malloc
         mov r4, r0
         add r0, r4, #0
         ldr r1, =C1$m
         str r1, [r0]
         ldr r1, [r5, #0]
         mov r0, r5
         blx r1
         mov r0, r4
         ldr r1, [r0, #0]
         blx r1
         mov r0, #0
         sub sp, fp, #32
         pop {r4-r10, fp, pc}

@ Here's function: C^m

.balign 4
.global C$m
.section .text

C$m:
         push {r4-r10, fp, lr}
         add fp, sp, #32
C$m$L100: 
         mov r0, #0
         bl putint
         mov r0, #10
         bl putch
         mov r0, #0
         sub sp, fp, #32
         pop {r4-r10, fp, pc}

@ Here's function: C1^m

.balign 4
.global C1$m
.section .text

C1$m:
         push {r4-r10, fp, lr}
         add fp, sp, #32
C1$m$L100: 
         mov r0, #1
         bl putint
         mov r0, #10
         bl putch
         mov r0, #0
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
