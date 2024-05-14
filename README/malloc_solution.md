# CSAPP  malloc lab实现思路
## 要求梳理
文档要求实现一个双字对齐的分配器，主要就是要求实现他给出的以下函数:
```C
int   mm_init(void);                       // 初始化
void *mm_malloc(size_t size);              // 分配堆中空间并返回一个指针
void  mm_free(void *ptr);                  // 释放空间
void *mm_realloc(void *ptr, size_t size);  // 改变指针所指的旧块大小
```
然后文档中列了几点要求：
+ 不能调用库函数
+ 不允许定义任何全局或静态的复合数据结构，例如arrays、lists、structs等
+ 分配器必须始终返回与8字节边界对齐的指针

当然只靠它给的四个函数是无法是实现分配器的，所以我们还要定义宏和辅助函数，当然它也给出了一些可调用的辅助函数，如下：
```c
void  *mem sbrk(int incr);   // 扩展和收缩堆
void  *mem heap lo(void);    // 堆顶指针
void  *mem heap hi(void);    // 堆底指针
size_t mem heapsize(void);   // 堆目前大小
size_t mem pagesize(void);   // 返回系统的页面大小（字节）（在Linux中为4K）。
```
## 知识回顾
讲到这的话，我们就先来回顾一下堆相关的知识
堆
## 基本思路
### 隐式空闲链表
### 分离的空闲链表
## 具体代码
### 隐式空闲链表
[完整代码见仓库](https://github.com/qaok/csapp/blob/master/malloclab/malloclab-handout/mm.c)
### 分离的空闲链表
## 代码测试
### 隐式空闲链表
### 分离的空闲链表
