This is Homework 8 assignment.

1) You need only to complete the code in simcoafrespisel.cc (under lib/quadflow), which consists of simplifying, coalescing, freezing, spilling, and selecting methods in the Coloring class (see include/quadflow/coloring.hh). There is a check color method call (provided also in the Coloring class) at the end of select which you should use to make sure your coloring code works correctly.

2) The input is a quad program in SSA form (examples are given under test/input_example directory, with the corresponding fmj files under test/fmj_exmpale), and the output should be a color (register allocation) file. Some examples are given under test/output_example. The output examples are given in k5 (5 colors) and k9 (9 colors) respectively. The output files are printed with Coloring::coloring2xml() function in the Coloring class.

3) The input quad programs are prepared for use for RPi (ARM) architecture targets. We will explain this preparation step in class. The preparation code is given under lib/rpi in prepareregalloc.cc. You need to follow the steps in the main.cc program and read the SSA Quad and prepare for Coloring with prepareregalloc.cc code.

4) The Interference graph class and the corresponding code to build the IG's are also given. Please see code and use the main.cc example.


03
08
01
09
fibonacci
06
05