#include  <os.h>
#include  "HL_Malloc.h"
//#include  <stddef.h>
#include  <assert.h>
/*5K 的全局变量供mem模块使用*/
MEM_BLK_DEF(32, 32);
MEM_BLK_DEF(16, 64);
MEM_BLK_DEF(8,  128);
MEM_BLK_DEF(4,  256);
MEM_BLK_DEF(2,  512);

const static MEM_INFO_t MEM_Tab[] =
{
	{"mem blk32 ", (CPU_INT08U*)MEM_BLK_N32_S32, 32,  32, &MEM_HDL_N32_S32},
	{"mem blk64 ", (CPU_INT08U*)MEM_BLK_N16_S64, 16,  64, &MEM_HDL_N16_S64},
	{"mem blk128", (CPU_INT08U*)MEM_BLK_N8_S128,  8, 128, &MEM_HDL_N8_S128},
	{"mem blk256", (CPU_INT08U*)MEM_BLK_N4_S256,  4, 256, &MEM_HDL_N4_S256},
	{"mem blk512", (CPU_INT08U*)MEM_BLK_N2_S512,  4, 512, &MEM_HDL_N2_S512},
};

/*
 * init memory pool
 */
   void  HL_MemInit(void)
   {
     OS_ERR err;
     for (int i = 0; i < ARR_NUM(MEM_Tab); i++)
     {
       OSMemCreate(MEM_Tab[i].handler, MEM_Tab[i].name, MEM_Tab[i].addr, MEM_Tab[i].n_blks, MEM_Tab[i].blk_size, &err);
       assert(OS_ERR_NONE == err);			
     }	
   }

/*
 * get memory handler by data len
 */
   char GetMemIdxByLen(int len)
   {
     int i;
     
     if (len > MEM_BLK_MAX_SIZE)
       return -1;
     
     for (i = 0; i < ARR_NUM(MEM_Tab); i++)
     {
       if (len <= MEM_Tab[i].blk_size)
       {
         return i;
       }
     }
     
     return (char)-2;
   }

/*
 * malloc based on memory index
 */
void* HL_MALLOC(int8_t mem_idx, int len)
{
	OS_ERR err;
    
    assert((mem_idx >= 0) && (mem_idx < ARR_NUM(MEM_Tab)));
    assert(len <= MEM_Tab[mem_idx].blk_size);
		
	return OSMemGet(MEM_Tab[mem_idx].handler, &err);
}

/*
 * free based on memory index
 */
void HL_Free(int8_t mem_idx, void* p_blk)
{
	OS_ERR err;
    
    assert((mem_idx>=0) && (mem_idx < ARR_NUM(MEM_Tab)));
    
	OSMemPut(MEM_Tab[mem_idx].handler, p_blk, &err);
	assert (OS_ERR_NONE == err);		
}


