#### 抽象语法树 (AST) 和 FDMJ-SLP 文档

复旦大学计算机专业2025年春季学期“编译(H)”

### 概述

本学期我们要编译的语言叫FDMJ（Fudan Mini-Java），我们从一个子集FDMJ-SLP（Straight Line Program）开始。抽象语法树 (AST) 是编译过程中至关重要的组件，表示源代码的层次结构。AST 由各种节点类型组成，每种节点类型对应源语言中的不同构造。FDMJ-SLP定义了多个类来表示这些节点及其关系。这里描述的的相关代码在课件Repo（gitee）的HW1之下（包括ASTHeader.hh，FDMJAST.hh，有些实现的细节在FDMJAST.cc文件里）。

### 关键组件

1. **AST 基类**
   - **目的**：作为 AST 中所有其他节点类型的根。
   - **成员**：
     - `Pos* pos`：节点的位置信息。
   - **方法**：
     - `Pos* getPos()`：返回位置信息。
     - `virtual void accept(AST_Visitor& v) = 0`：接受访问者访问此节点。
     - `virtual ASTKind getASTKind() = 0`：返回 AST 节点的类型。
     - `virtual AST* clone() = 0`：克隆 AST 节点。

2. **Pos 类**
   - **目的**：存储 AST 节点的位置信息（起始和结束行和列）。
   - **成员**：
     - `size_t sline, scolumn, eline, ecolumn`：起始和结束行和列。
   - **方法**：
     - `Pos* clone()`：克隆位置对象。

3. **AST_Visitor 类**
   - **目的**：访问不同类型 AST 节点的抽象类。
   - **方法**：
     - `virtual void visit(Program *node) = 0`
     - `virtual void visit(MainMethod *node) = 0`
     - `virtual void visit(Stm *node) = 0`
     - `virtual void visit(Exp *node) = 0`

4. **ASTKind 枚举**
   - **目的**：枚举不同类型的 AST 节点。
   - **值**：
     - `Program`, `MainMethod`, `Assign`, `Return`, `BinaryOp`, `UnaryOp`, `Esc`, `IdExp`, `OpExp`, `IntExp`, `StrConst`, `IntConst`, `Unknown`

5. **Program 类**
   - **目的**：表示整个程序。
   - **成员**：
     - `MainMethod* main`：程序的主方法。
   - **方法**：
     - `ASTKind getASTKind() override`：返回 `ASTKind::Program`。
     - `Program* clone() override`：克隆 Program 节点。
     - `void accept(AST_Visitor &v) override`：接受访问者。

6. **MainMethod 类**
   - **目的**：表示程序中的主方法。
   - **成员**：
     - `StmList* sl`：主方法中的语句列表。
   - **方法**：
     - `ASTKind getASTKind() override`：返回 `ASTKind::MainMethod`。
     - `MainMethod* clone() override`：克隆 MainMethod 节点。
     - `void accept(AST_Visitor &v) override`：接受访问者。

7. **Stm 类**
   - **目的**：所有语句的基类。语句的类别有两种：Assign和Return。每种语句都由子语句与子表达式来表达。其中Assign由两个表达式（赋值语句的左边与右边，而左边一定是一个简单的变量），Return由一个子表达式组成。**（注意在FDMJ-SLP中，子语句不起作用，保留之后使用。）**
   - **成员**：
     - `ASTKind stmKind`：语句的类型。
     - `StmList* sl`：子语句列表。
     - `ExpList* el`：表达式列表。
   - **方法**：
     - `ASTKind getASTKind() override`：返回语句的类型。
     - `Stm* clone() override`：克隆语句节点。
     - `void accept(AST_Visitor &v) override`：接受访问者。

8. **Exp 类**
   - **目的**：所有表达式的基类。表达式的类别有：BinaryOp, UnaryOp, Esc, IdExp, OpExp, 和IntExp。其中：
	   - BinaryOP用三个子表达式表达（Exp1，OpExp，Exp2）
	   - UnaryOp（OpExp，Exp）
	   - Esc由一串子语句（sl）和一个Exp表达
	   - IdExp，OpExp和IntExp没有子语句和子表达式，相关的值表达在val（一个variant）里，其中IdExp和OpExp使用StrConst\*（里面有一个string），IntConst\*（里有一个int）。
   - **成员**：
     - `ASTKind expKind`：表达式的类型。
     - `vector<Stm*> *sl`：子语句列表。
     - `vector<Exp*> *el`：表达式列表。
     - `variant<std::monostate, IntConst*, StrConst*> val`：某些表达式的值。
   - **方法**：
     - `ASTKind getASTKind() override`：返回表达式的类型。
     - `Exp* clone() override`：克隆表达式节点。
     - `void accept(AST_Visitor &v) override`：接受访问者。

9. **IntConst 类**
   - **目的**：表示一个整数常量。跟所有AST节点一样，都有一个Pos\*成员。
   - **成员**：
     - `int val`：整数值。
   - **方法**：
     - `ASTKind getASTKind() override`：返回 `ASTKind::IntConst`。
     - `IntConst* clone() override`：克隆 IntConst 节点。
     - `void accept(AST_Visitor &v) override`：不操作（不访问）。

10. **StrConst 类**
    - **目的**：表示一个字符串常量。
    - **成员**：
      - `string str`：字符串值。
    - **方法**：
      - `ASTKind getASTKind() override`：返回 `ASTKind::StrConst`。
      - `StrConst* clone() override`：克隆 StrConst 节点。
      - `void accept(AST_Visitor &v) override`：不操作（不访问）。

### 辅助函数

- **fdmjParser**
  - 解析文件并返回一个 `Program` 对象。
  - 对 `std::string` 文件名和 `std::ifstream` 进行重载。

- **stringASTKind**
  - 将 `ASTKind` 枚举值转换为字符串表示。

此文档概述了 AST 和 FDMJ 框架内的关键组件及其关系。每个类和方法的设计旨在促进 AST 的构建、遍历和操作，从而实现源代码的高效编译和分析。
