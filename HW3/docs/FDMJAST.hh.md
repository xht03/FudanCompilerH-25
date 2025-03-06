Here's the documentation for the classes in the FDMJAST.hh file:

### Program Structure Classes

**Program Class**
- Represents the entire program
- Contains:
  - MainMethod: The main method of the program
  - ClassDecl: List of class declarations

**MainMethod Class**
- Represents the main method of the program
- Contains:
  - VarDecl: List of variable declarations
  - Stm: List of statements

**ClassDecl Class**
- Represents a class declaration
- Contains:
  - IdExp: Class identifier
  - IdExp: Extended class identifier (for inheritance)
  - VarDecl: List of class variables
  - MethodDecl: List of methods

### Type and Declaration Classes

**Type Class**
- Represents type information
- Contains:
  - TypeKind: Type classification (INT, CLASS, ARRAY)
  - IdExp: Class identifier (for CLASS type)
  - IntExp: Array arity (for ARRAY type)

**VarDecl Class**
- Represents variable declarations
- Contains:
  - Type: Variable type
  - IdExp: Variable identifier
  - Initialization value (IntExp or array of IntExp)

**MethodDecl Class**
- Represents method declarations
- Contains:
  - Type: Return type
  - IdExp: Method identifier
  - Formal: List of formal parameters
  - VarDecl: List of local variables
  - Stm: List of statements

**Formal Class**
- Represents formal parameters in method declarations
- Contains:
  - Type: Parameter type
  - IdExp: Parameter identifier

### Statement Classes

**Stm (Base Class)**
- Base class for all statement types

**Control Flow Statements:**
- **If:** If-else statement
- **While:** While loop statement
- **Nested:** Block of statements
- **Continue:** Continue statement
- **Break:** Break statement
- **Return:** Return statement

**Assignment and Call Statements:**
- **Assign:** Assignment statement
- **CallStm:** Method call statement

**I/O Statements:**
- **PutInt:** Print integer statement
- **PutCh:** Print character statement
- **PutArray:** Print array statement
- **GetInt:** Read integer statement
- **GetCh:** Read character statement
- **GetArray:** Read array statement

**Timing Statements:**
- **Starttime:** Start timing statement
- **Stoptime:** Stop timing statement

### Expression Classes

**Exp (Base Class)**
- Base class for all expression types

**Operators:**
- **BinaryOp:** Binary operation expression
- **UnaryOp:** Unary operation expression
- **OpExp:** Operator representation

**Access Expressions:**
- **ArrayExp:** Array access expression
- **ClassVar:** Class variable access expression
- **CallExp:** Method call expression
- **Length:** Array length expression

**Literal Expressions:**
- **BoolExp:** Boolean literal
- **IntExp:** Integer literal
- **IdExp:** Identifier expression

**Special Expressions:**
- **This:** 'this' reference
- **Esc:** Expression with statement block

### Common Features
All classes:
- Inherit from AST base class
- Include position information (Pos)
- Implement clone() method for deep copying
- Support visitor pattern through accept() method
- Have unique ASTKind identifier
