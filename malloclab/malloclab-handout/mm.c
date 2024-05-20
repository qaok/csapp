/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "killmalloc",
    /* First member's full name */
    "qaok",
    /* First member's email address */
    "201644606@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 基本参数设置
#define WSIZE        4                         // 字大小以及头部、脚部大小
#define DSIZE        8                         // 双字大小
#define CHUNKSIZE   (1 << 12)                  // 堆扩展的默认大小

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))   // 将块大小和分配位进行或运算

#define GET(p)      (*(unsigned int *) (p))    // 读指针p的位置        
#define PUT(p, val) ((*(unsigned int *)(p)) = (val)) // 写指针p的位置

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


#define HEAD(num)     (heap_listp + WSIZE * num)
#define GET_HEAD(num) ((char *)(long)(GET(HEAD(num))))
#define GET_PREV(bp)  ((char *)(long)(GET(bp)))
#define GET_NEXT(bp)  ((char *)(long)(GET((char *)bp + WSIZE)))
    
    
#define LIST_SIZE 16
#define INIT_SIZE (LIST_SIZE * WSIZE)    
static char* free_list[LIST_SIZE];
static char *heap_listp;

// 辅助函数
static void *extend_heap(size_t words); 
static void *coalesce(void *bp);           
static void *find_fit(size_t asize);
static void *best_fit(size_t asize);
static void  place(void *bp, size_t asize); 
static void  delete(void *bp);              
static void  insert(void *bp);              
static int   search(size_t size);           


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    if((heap_listp = mem_sbrk(4*WSIZE + INIT_SIZE)) == (void *)-1)
        return -1;
 
    for(int i = 0; i < LIST_SIZE; i++){
        free_list[i] = HEAD(i);
        PUT(free_list[i], NULL);
    }
    PUT(heap_listp + INIT_SIZE, 0);
    PUT(heap_listp + WSIZE + INIT_SIZE, PACK(DSIZE, 1));   
    PUT(heap_listp + 2*WSIZE + INIT_SIZE, PACK(DSIZE, 1));    
    PUT(heap_listp + 3*WSIZE + INIT_SIZE, PACK(0, 1));      


    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t asize;
    size_t extendsize;
    char *bp;
    if(size == 0)
        return NULL;
    // 此处表明最小块大小必须是16字节，8字节给头部和脚部，8字节满足对齐要求
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    if((bp = best_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }
    
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    if(ptr==0)
        return;
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *newptr;
    size_t copysize;
    
    if((newptr = mm_malloc(size))==NULL)
        return 0;
    copysize = GET_SIZE(HDRP(ptr));
    if(size < copysize)
        copysize = size;
    memcpy(newptr, ptr, copysize);
    mm_free(ptr);
    return newptr;
}

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

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));  // 前一个块的分配位 
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  // 后一个块的分配位 
    size_t size = GET_SIZE(HDRP(bp));                    // 当前块的大小     

    // 情况1：前面和后面块都已分配，直接返回          
    if (prev_alloc && next_alloc) {
        insert(bp);
        return bp;
    }

    // 情况2：前面块已分配，后面块空闲 
    else if (prev_alloc && !next_alloc) {
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));           // 当前块的大小加上后面块的大小
        PUT(HDRP(bp), PACK(size, 0));                    // 将后面块的头部大小设置为总和，分配位空闲
        PUT(FTRP(bp), PACK(size, 0));                    // 将后面块的脚部大小设置为总和，分配位空闲     
    } 

    // 情况3：后面块已分配，前面块空闲 
    else if (!prev_alloc && next_alloc) {
        delete(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));           // 当前块的大小加上前面块的大小
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp); 
    }
 
    // 情况4：前面和后面块都空闲
    else {
        delete(NEXT_BLKP(bp));
        delete(PREV_BLKP(bp));
        // 前面块和后面块的大小都加上
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        
    }
    insert(bp);
    return bp;
}

// 首次适配
static void *find_fit(size_t asize) {
    int num = search(asize);
    char *bp;
    for (; num < LIST_SIZE; num++) {
        for(bp = GET_HEAD(num); bp != NULL; bp = GET_NEXT(bp)) {
            if (GET_SIZE(HDRP(bp)) >= asize) {
                return bp;
            }
        }
    }
    return NULL;
}

static void *best_fit(size_t asize) {
    size_t size_gap = 1 << 30;
    int num = search(asize);
    char *bp;
    char *best_addr = NULL, *temp;
    for (; num < LIST_SIZE; num++) {
        for(bp = GET_HEAD(num); bp != NULL; bp = GET_NEXT(bp)) {
            temp = HDRP(bp);
            if (GET_SIZE(temp) - asize < size_gap) {
                size_gap = GET_SIZE(temp) - asize;
                best_addr = bp;
                if (GET_SIZE(temp) == asize) return best_addr;
            }
        }
    }
    return best_addr;
}
 


static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    size_t remain_size = csize - asize;
    delete(bp);
    // 当请求块的大小与当前块大小之差大于16字节时，才进行分割
    if (remain_size >= 2*DSIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(remain_size, 0));
        PUT(FTRP(bp), PACK(remain_size, 0));

        insert(bp);
    }
    
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


static void delete(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int num = search(size);
    char* prev = GET_PREV(bp);
    char* next = GET_NEXT(bp);

    if (prev == NULL && next == NULL) {
        PUT(free_list[num], NULL);
    }

    else if (prev != NULL && next == NULL) {
        PUT(prev + WSIZE, NULL);
    }

    else if (prev == NULL && next != NULL) {
        PUT(free_list[num], next);
        PUT(next, NULL);
    }
    
    else if (prev != NULL && next != NULL) {
        PUT(prev + WSIZE, next);
        PUT(next, prev);
    }
}


static void insert(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int num = search(size);
    if (GET_HEAD(num) == NULL) {
        PUT(free_list[num], bp);
        PUT(bp, NULL);
        PUT((char *)bp + WSIZE, NULL);
    } else {
        PUT((char *)bp + WSIZE, GET_HEAD(num));
        PUT(GET_HEAD(num), bp);
        PUT(bp, NULL);
        PUT(free_list[num], bp);
    }
}


static int search(size_t size) {
    for (int i = 4; i <= 18; i++) {
        if (size <= (1 << i))
            return i - 4;
    }
    return 15;
}


