#ifndef __HL_MALLOC_H__
#define __HL_MALLOC_H__

#include <stdint.h>
/*基于ucos-iii的内存管理*/

/*此malloc共管理内存 5K*/
#define MEM_BLK_MAX_SIZE 512
#define ARR_NUM(A)  (sizeof(A)/sizeof((A)[0]))

#define MEM_BLK_DEF(num, size) \
    static CPU_INT08U MEM_BLK_N##num##_S##size[num][size]; \
    static OS_MEM	  MEM_HDL_N##num##_S##size

typedef struct
{
	char*		 name;
	CPU_INT08U	*addr;
	OS_MEM_QTY	 n_blks;
	OS_MEM_SIZE	 blk_size;
    OS_MEM      *handler;
}MEM_INFO_t;

typedef enum
{
    MEM_BLK_IDX_SIZE_32 = 0,
    MEM_BLK_IDX_SIZE_64,
    MEM_BLK_IDX_SIZE_128,
    MEM_BLK_IDX_SIZE_256,
    MEM_BLK_IDX_SIZE_512,
} MEM_BLK_IDX_E;
    
void  HL_MemInit(void);
char  GetMemIdxByLen(int len);
void* HL_MALLOC(int8_t mem_idx, int len);
void  HL_Free(int8_t mem_idx, void* p_blk);



#endif