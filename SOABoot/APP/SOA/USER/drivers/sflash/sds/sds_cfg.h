#ifndef __SDS_CFG_H__
#define __SDS_CFG_H__

#ifdef __cplusplus 
extern "C" {
#endif



extern const SDS_Item_t SDS_Table[];
    
#define SDS_TEMP_BUF_SIZE          64

#define SDS_MEM_ADDR           0x0080000  //128K
#define SDS_MEM_SIZE           0x40000  //64K

#define SDS_ITEMS_NB              2     /* SDS 数据项个数 */

#ifdef __cplusplus
}
#endif

#endif /* __SDS_CFG_H__ */