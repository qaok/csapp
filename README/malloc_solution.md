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
void  *mem sbrk(int incr);                 // 扩展和收缩堆
void  *mem heap lo(void);                  // 堆顶指针
void  *mem heap hi(void);                  // 堆底指针
size_t mem heapsize(void);                 // 堆目前大小
size_t mem pagesize(void);                 // 返回系统的页面大小（字节）（在Linux中为4K）。
```
## 知识回顾
讲到这的话，我们就先来回顾一下`堆`相关的知识
### 堆与虚拟内存
+ 每个linux进程都有属于自己的单独的虚拟地址空间，主要分为栈、共享库、堆以及.bss文件（未初始化）和.data文件（已初始化）还有代码段
+ 分配器是将堆视为一组不同大小的`块（block）`的集合来维护，每个块就是一个连续的`虚拟内存片（chunk）`，要么是已分配的，要么是空闲的，空闲块可用来分配
+ 堆是通过malloc函数（显式分配器）实现分配内存的，堆的内存并不像栈一样会被系统释放，堆通过mallo函数所分配的内存需要手动释放，如果存在过多已经不再使用的堆内存没有被释放的话，会导致堆内存溢出
![虚拟内存图片](images/b.png)

### 堆块的格式
考虑到堆块合并问题，我们此处选择`带边界标记的堆块`格式
+ 由三部分构成，头部、有效载荷和填充、脚部
+ 头部和脚部均表示块大小，且脚部与下一块的头部只差一字
+ 指针从有效载荷处开始
+ 由于是8字节对齐，后三位均为0，最后一位（`分配位`）表示当前块是已分配还是空闲的
![堆块图片](images/d.png)

### 块序列
1. 隐式空闲链表
2. 显式空闲链表
3. 分离的空闲链表
两种基本方法：
+ 简单分离存储
+ 分离适配

### 放置策略选择
#### 1. 首次适配
+ 需从头开始搜索链表从而找到合适的空闲块，时间过长
+ 易将大的空闲块保留在链表后面，也易导致起始处出现过多`小碎片（外部碎片）`
#### 2. 下一次适配
+ 时间比首次适配快，但内存利用率低得多
#### 3. 最佳适配
+ 内存利用率高，但容易出现对堆进行彻底的搜索的情况

## 基本思路
我们主要就是考虑选择什么样的块序列及放置策略
### 1. 基于隐式空闲链表
书上给出了基于`隐式空闲链表`，使用`立即边界标记合并`方式，选择`首次适配`的策略实现了一个简单分配器，我们来看一下代码：
```c
#define WSIZE        4                         // 字大小以及头部、脚部大小
#define DSIZE        8                         // 双字大小
#define CHUNKSIZE   (1 << 12)                  // 堆扩展的默认大小

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))   // 将块大小和分配位进行或运算

#define GET(p)      (*(unsigned int *) (p))    // 读指针p的位置        
#define PUT(p, val) (*(unsigned int *) (p) = (val)) // 写指针p的位置

#define GET_SIZE(p)  (GET(p) & ~0x7)           // 得到块的大小
#define GET_ALLOC(p) (GET(p) & 0x1)            // 块是否已分配

// 给定块指针bp，得到它头部的指针
#define HDRP(bp) ((char *)(bp) - WSIZE)        
// 给定块指针bp，得到它脚部的指针
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

// 给定块指针bp，得到它下一个块的指针
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
// 给定块指针bp，得到它前一个块的指针
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
```
这里感觉容易混淆，补充说明下：
+ GET_SIZE中用到的指针p指向`头部或脚部`，从而得到当前块大小
+ HDRP、FTRP、NEXT_BLKP、PREV_BLKP的指针bp（block pointer）则指向`有效载荷`

接下来就是我们基于隐式空闲链表所要实现的函数了，书上给出的函数如下：
```c
static void *extend_heap(size_t words);       // 用一个新的空闲块对堆进行扩展 
static void *coalesce(void *bp);              // 合并空闲块
static void *find_fit(size_t asize);          // 首次适配
static void  place(void *bp, size_t asize);   // 将请求块放置在空闲块的起始位置
```
#### 具体代码
由于书上给出的整体实现代码过长，就不一一贴出，挑几个函数讲一下
1. extend_heap函数
```c
static void *extend_heap(size_t words) {
    char  *bp;
    size_t size;
    // 保证扩展的大小是是双字对齐的
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // 改变指针bp位置，如果堆没有足够的空闲块了，就返回-1
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    PUT(HDRP(bp), PACK(size, 0));             // 写入空闲块头部      
    PUT(FTRP(bp), PACK(size, 0));             // 写入空闲块脚部
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));     // 将该空闲块的下一块的头部大小设定为0，且分配位设置为已分配

    // 未扩展前的堆可能以空闲块结尾，合并这两个空闲块，并返回块指针bp
    return coalesce(bp);                      
}

```
这个函数主要就是用来获得新空闲块来扩展堆，它一般在两种情况下被用到：
+ 堆被初始化时
+ mm_malloc函数没法找到一个合适的空闲块时，需要像内存系统请求额外的堆空间
2. coalesce函数
```c
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));  // 前一个块的分配位 
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  // 后一个块的分配位 
    size_t size = GET_SIZE(HDRP(bp));                    // 当前块的大小     

    // 情况1：前面和后面块都已分配，直接返回          
    if (prev_alloc && next_alloc) {  
        return bp;
    }

    // 情况2：前面块已分配，后面块空闲 
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));           // 当前块的大小加上后面块的大小
        PUT(HDRP(bp), PACK(size, 0));                    // 将后面块的头部大小设置为总和，分配位空闲
        PUT(FTRP(bp), PACK(size, 0));                    // 将后面块的脚部大小设置为总和，分配位空闲     
    }

    // 情况3：后面块已分配，前面块空闲 
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));           // 当前块的大小加上前面块的大小
        PUT(FTRP(bp), PACK(size, 0));                    // 将前面块的脚部大小设置为总和，分配位空闲
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));         // 将前面块的头部大小设置为总和，分配位空闲
        bp = PREV_BLKP(bp);                              // 改变块指针bp位置，将其从当前块的有效载荷移到前面块的有效载荷处
    }
 
    // 情况4：前面和后面块都空闲
    else {
        // 前面块和后面块的大小都加上
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));  
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));         // 将后面块的脚部大小设置为总和，分配位空闲     
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));         // 将前面块的头部大小设置为总和，分配位空闲
        bp = PREV_BLKP(bp);                              // 改变块指针bp位置，将其从当前块的有效载荷移到前面块的有效载荷处
    }
    return bp;
}

```
+ 带有边界标记的块利于我们合并，但是由于需保持多个头部和脚部，会带来显著的内存开销
+ 隐式空闲链表在分配请求块时，搜索时间过长，花费成本过高
+ 立即合并空闲块会导致每释放一个块，就会合并相邻块，有时候会出现抖动，块反复合并又马上分割

[完整代码见仓库](https://github.com/qaok/csapp/blob/master/malloclab/malloclab-handout/mm.c)
### 2. 基于分离的空闲链表

### 隐式空闲链表

### 分离的空闲链表
[完整代码见仓库](https://github.com/qaok/csapp/blob/master/malloclab/malloclab-handout/mm.c)
## 代码测试
### 1. 隐式空闲链表
测试结果如下：

只得到50分，效果很一般，因此我们不考虑使用这种方法
![堆块图片](images/e.png)
### 2. 分离的空闲链表
