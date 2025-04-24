# lab6: Translate IRP2Quad

在HW4-5中，我们将AST转换成了一种中间语言TigerIR+，其本身还是以树的形式来呈现代码。本次作业，我们需要将TigerIR+转换成Quad。

# tiling

相比于TigerIR+，Quad在形式上更接近汇编语言。Quad是一条一条语句排列在一起的形式，可以把一条语句看成一块砖头（tile）。

我们可以把固定形状的TigerIR+树的一部分用一条特定的Quad语句来实现（PPT Week9 第11页），最后整棵树会变成一串砖头连在一起，所以这个过程也叫做铺砖（tiling）。

# def和use

一条Quad语句可能会使用某些之前赋值的变量，也有可能会赋值一个新的变量，前者称为use，后者称为def。

从Quad程序到汇编程序，如果把每一段语句看成一个结点，整个程序就可以看作是一个有向图。在tiling的同时我们可以标注每个Jump和CJump的目的地，从而分析出这张有向图上哪两个点连了一条边（Control Graph）；也可以标注出每条语句的def和use，从而分析出每个变量在这张图上的流动方向、使用范围（Data Flow Graph），为后续转换SSA等做准备。

# lab6任务

你需要修改`lib/quad/tree2quad.cc`，填充`tree2quad`和各`visit`函数的内容。你可以使用`temp_map`来生成新的变量，将visit的结果以一个`QuadStm*`的`vector`来表示，并在构造每个`QuadStm`的时候标注其`def`和`use`了什么变量，跳转到了什么标签。