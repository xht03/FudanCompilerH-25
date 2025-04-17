This is HW4 (published on March 22, 2025).


1. Goal: You are to code the ast2tree program that translates a semantically correct FDMJ program (in AST form) to a tree program. 

2. The input to your program (see main.cc) is an AST XML file (with semantic information added, named "file.2-semant.ast"). The semantic information is built into an AST_Semant_Map when the AST XML file is read (see xml2ast.cc). For more test files, you may use the program in vendor (named "gentests_xx_xx" file,  note it's a binary file and you need to use the correct version) to generate AST XML files from FDMJ source files.

3. For output, you should use tree2xml to convert your tree to an XML file (named "file.3.irp").

4. The use of Tr_Exp etc. will be explained in class.
