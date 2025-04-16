# lab4&5: Translate AST2IRP

> HW1-HW3的文档提供了保姆式的实现细节带大家熟悉项目，HW4开始的文档不会提供具体的实现细节，需要仔细看书、认真听课、自己思考。【见 `lab1.md`的 `项目介绍-注意事项`！】

请将实现细节详细地写在实验报告里，主要内容见下文的**Q**，风格可以参考HW1-HW3的文档。此外，不要忘了 `lab1.md`中要求的报告的其他内容。

- 在HW4中，做的是不带array和class的AST2IRP，报告中只需要包含相关**Q**的回答即可。
- 在HW5中，做的是全部的AST2IRP，需要续写HW4报告的内容，回答以下所有的**Q**。

# Tiger IR+ (IRP) 的设计

tiger以exp为主体，exp里可以有stm；FDMJ是以stm为主体，里边可以有exp

我们在Tiger IR的基础上加了点东西变成Tiger IR+使之适配FDMJ

**Hint**: Tiger IR设计请见虎书，Tiger IR+的设计请见上课PPT

**Q1.1**: `treep.hh`中有许多tigerirp的 `class`，他们分别起到了什么作用？请用中文语言描述（不要包含大段代码），并在实验报告中包含。例如：

- `tree::ExpStm`用来忽略 `tree::Exp`的返回值让 `tree::Exp`变成 `tree::Stm`。
- `tree::Label`作为程序跳转的目的地，类似于goto标签的作用。
- `tree::TempExp`用于转换id（`tree::Temp`）为 `tree::Exp`
- `tree::Const `用于转换num（`int`）为 `tree::Exp`

**Q1.2**: 相对于虎书中的Tiger IR，我们的Tiger IR+多了哪些内容，为什么需要多的这些内容？请用中文语言描述（不要包含大段代码），并在实验报告中包含。

# Translate的方法

## 不带class时

**Hint**: `Patch_list`和 `Tr_Exp`细节请见虎书和上课PPT

**Q2**: 在不带class的翻译情况下，需要关注运算（算数运算、比较运算、逻辑运算……）、赋值、条件（if、while）、数组（初始化、赋值、存取、长度、运算……）等成分的翻译，你是如何完成它们的翻译的？请用中文语言描述（不要包含大段代码），并在实验报告中包含。例如：

- 数组初始化：
  - 对于形如 `int[] a={ConstList}`的语句，调用 `std::new`分配比数组长度多1的空间，每个位置的类型为 `int`，在-1位置放置数组长度，然后通过计算偏移并通过使用 `tree::Mem`为其他位置赋值。
  - ……
- 数组存取（`exp[exp]`）：计算偏移，然后通过 `tree::Mem`取对应位置的内容进行读取或赋值
- 数组长度（`length(exp)`）：通过 `tree::Mem`取-1位置获得长度

## 带class时

在FDMJ中，method和function是一个概念，在翻译过程中会消除class的概念，提取method，重命名表示不同类的method。程序以function为单位，程序用一系列function表示（a list of functions），第一个是main（约定）；每个function有（重命名后的）函数名、参数列表（a list of temps，之后到机器语言里会变成一堆栈）、语句（stms；变量没有声明，都是temp的形式，temp可以认为是reg，有value/pointer和location，在IR里有无限个）。

**Hint**: Unified Object Record细节请见上课PPT

**Q3**: 你是如何重命名method的？main method和class method的参数列表有何不同（hint：`this`）？你是如何处理class method中的 `this`的？你是如何记录不同class的变量和方法的（hint：Unified Object Record）？你是如何处理多态的？你是如何翻译有关class的操作（初始化、访问类变量、访问类方法……）的？请用中文语言描述（不要包含大段代码），并在实验报告中包含。
