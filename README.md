# CSAPP labs概述
## 1. Data lab
### 实现要求
主要有以下一些说明：
1. 整型的范围是 0 到 255(0xFF)，不允许用更大
2. 只能包含参数和局部变量
3. 一元操作符 ! ~
4. 二元操作符 & | + << >>

不允许的操作有：
1. 使用任何条件控制语句
2. 定义和使用宏
3. 定义其他的函数
4. 调用函数
5. 使用其他的操作符
6. 使用类型转换
7. 使用除 int 之外的类型（针对整型）
8. 使用除 int, unsigned 之外的类型（针对浮点数）
### 相关知识
关于`整数和浮点数的位运算`，目的是通过这次的作业来熟悉整型及浮点数的位表达形式
## 2. Bomb lab
### 实现要求
这次的任务一共有七关，六个常规关卡和一个隐藏关卡，每次我们需要输入正确的拆弹密码才能进入下一关，而具体的拆弹密码藏在汇编代码中。

主要是解译提供给我们的汇编代码，然后通过汇编代码来写出c代码，目的就是为了加深对汇编的理解。
### 相关知识
主要是关于`汇编`的知识，`寄存器、内存地址、立即数`等相关内容。
## 3. Attack lab
### 实现要求
这一次我们将实现两种不同类型的攻击：
1. 缓冲区溢出攻击
2. ROP 攻击

利用`缓冲区溢出，实际上是通过重写返回值地址，来执行另一个代码片段，就是所谓代码注入了`。比较关键的点在于：
1. 熟悉 x86-64 约定俗成的用法
2. 使用 objdump -d 来了解相关的偏移量
2. 使用 gdb 来确定栈地址
4. 机器使用小端序

另一种攻击是使用 `return-oriented programming` 来执行任意代码，这种方法在 stack 不可以执行或者位置随机的时候很有用。

这种方法主要是`利用 gadgets 和 string 来组成注入的代码`。具体来说是使用 pop 和 mov 指令加上某些常数来执行特定的操作。也就是说，`利用程序已有的代码，重新组合成我们需要的东西`，这样就绕开了系统的防御机制。
### 相关知识
汇编和栈相关的知识。进一步加深对于汇编的理解，同时对栈有更好的理解。
## 4. Cache lab
### 实现要求
这次实验的任务很明确，就是制作自己的缓存系统，具体来说是:
1. 实现一个缓存模拟器，根据给定的 trace 文件来输出对应的操作
2. 利用缓存机制加速矩阵运算
### 相关知识
`高速缓存`相关知识。对高速缓存的运行原理和内在机制有更好的了解。
## 5. Shell lab
### 实现要求
这次的实验，我们需要自己完成一个简单的 shell 程序。
### 相关知识
`异常控制流和进程`相关知识。通过具体的实现，我们可以更加深入地计算机运行的机制（尤其是 Exceptional Control Flow 和进程）
## 6. Malloc lab
### 实现要求
这次我们会实现自己的 malloc, free, realloc, calloc 函数，并借此深入理解堆中的内存分配机制。
### 相关知识
`虚拟内存和动态存储分配`相关知识。
### 解决思路
[本人的具体实现思路在此](https://blog.csdn.net/m0_61736858/article/details/139102260)
## 7. Proxy lab