#include  <stddef.h>
#include "hl_msgdeal.h"
#include "hl_malloc.h"
#include <assert.h>

//MEM_BLK_DEF(32,  sizeof(MSG_BUF_t));
MEM_BLK_DEF(32,  24);

/*
 * init message memory
 */
static void MsgMem_Init()
{
	OS_ERR err;
    OSMemCreate(&MEM_HDL_N32_S24, "msg mem", MEM_BLK_N32_S24, 32, sizeof(MSG_BUF_t), &err);
    assert(OS_ERR_NONE == err);	
}

/*
 * malloc message
 */
static void *MsgMem_Malloc()
{
	OS_ERR err;
    return OSMemGet(&MEM_HDL_N32_S24, &err);
}

/*
 * free message
 */
static void MsgMem_Free(void* p_blk)
{
	OS_ERR err;
    OSMemPut(&MEM_HDL_N32_S24, p_blk, &err);
	assert(OS_ERR_NONE == err);
}

void MsgDeal_Init(void)
{
	HL_MemInit();
	MsgMem_Init();
}

/*
 * send message
 */
int MsgDeal_Send(MessageAddr srcID, MessageAddr dstID, MSG_ID_TYPE msgid, void *pbuf, int len)
{
  MSG_BUF_t *msg = NULL;
  char      *buf = NULL;
  OS_ERR     err;
  int        ret = 0;
  
  assert ((dstID != NULL) && (len <= 512));
  
  msg = (MSG_BUF_t*)MsgMem_Malloc();
  if (NULL == msg)
  {
    ret = -1;
    goto err_handler;
  }
  
  msg->mtype = msgid;
  msg->srcID = srcID;
  msg->dstID = dstID;
  if (0 == len)
  {
    msg->reserved = 0;
    msg->len = 0;
    msg->buf = NULL;        
  }
  else
  {
    char idx;
    
    idx = GetMemIdxByLen(len);
    buf = HL_MALLOC(idx, len);
    if (NULL == buf)
    {
      ret = -2;
      goto err_handler;
    }
    
    msg->reserved = idx;
    msg->len = len;
    msg->buf = buf;
    Mem_Copy(buf, pbuf, len);
  }
  
  OSQPost(dstID, msg, sizeof(MSG_BUF_t), OS_OPT_POST_FIFO, &err);
  if (err != OS_ERR_NONE)
  {
    ret = -3;
    goto err_handler;
  }
  
  return 0;
  
err_handler:
  if (buf != NULL)
  {
    HL_Free(msg->reserved, buf);
    buf = NULL;
  }
  
  if (msg != NULL)
  {
    MsgMem_Free(msg);
    msg = NULL;
  }
  
  return ret;
}

/*
 * send message
 */
int MsgDeal_SendExt(MessageAddr dstID, MSG_BUF_t *msg)
{
	OS_ERR      err;
    assert ((dstID != NULL) && (msg != NULL));
    
    OSQPost(dstID, msg, sizeof(MSG_BUF_t), OS_OPT_POST_FIFO, &err);
	if (err != OS_ERR_NONE)
    {
        //MsgDeal_Free(msg);
        return -1;
    }
    return 0;    
}

///*
// * receive message
// */
//int MsgDeal_Recv(MessageAddr dstID, MSG_BUF_t **msg, int timeout)
//{
//  MSG_BUF_t  *pbuf  = NULL;
//  OS_MSG_SIZE Qsize = 0;
//  OS_ERR      err;
//  
//  assert ((dstID != NULL) && (msg != NULL));
//  
//  pbuf = (MSG_BUF_t*)OSQPend(dstID, timeout, OS_OPT_PEND_BLOCKING, &Qsize, NULL, &err);
//  if (err != OS_ERR_NONE)
//  {
//    return -1;
//  }
//  
//  assert(Qsize == sizeof(MSG_BUF_t));
//  
//  *msg = pbuf;
//  
//  return 0;
//}

/*
 * receive message
 * timeout : 0 --> OS_OPT_PEND_NON_BLOCKING
 *           -1 --> real timeout = 0 && OS_OPT_PEND_BLOCKING
 *           others --> real timeout = timeout && OS_OPT_PEND_BLOCKING
 */
int MsgDeal_Recv(MessageAddr dstID, MSG_BUF_t **msg, int timeout)
{
    MSG_BUF_t  *pbuf  = NULL;
    OS_MSG_SIZE Qsize = 0;
    OS_ERR      err;
    
  assert ((dstID != NULL) && (msg != NULL));
  OS_TICK real_timeout = (timeout > 0) ? timeout : 0;
  OS_OPT opt = (timeout == 0) ? OS_OPT_PEND_NON_BLOCKING : OS_OPT_PEND_BLOCKING ;
  
  pbuf = (MSG_BUF_t*)OSQPend(dstID, real_timeout, opt, &Qsize, NULL, &err);
  if (err != OS_ERR_NONE)
  {
    return -1;
  }
    
  assert(Qsize == sizeof(MSG_BUF_t));
    
  *msg = pbuf;
    
  return 0;
}

/*
 * free message
 */
void MsgDeal_Free(MSG_BUF_t *msg)
{
  assert (msg != NULL);
  
  if (msg->buf != NULL)
  {
    HL_Free(msg->reserved, msg->buf);
    msg->buf = NULL;
  }
  
  MsgMem_Free(msg);
  msg = NULL;
}

void MsgDeal_Clr(MessageAddr addr)
{
  assert (addr != NULL);
  
  MSG_BUF_t  *pbuf  = NULL;
  OS_MSG_SIZE Qsize = 0;
  OS_ERR      err;
  
  while(1)
  {
    pbuf = (MSG_BUF_t*)OSQPend(addr, 0, OS_OPT_PEND_NON_BLOCKING, &Qsize, NULL, &err);
    if (err != OS_ERR_NONE)
    {
      return ;
    }
    if (pbuf->buf != NULL)
    {
      HL_Free(pbuf->reserved, pbuf->buf);
      pbuf->buf = NULL;
    }    
    MsgMem_Free(pbuf);
    pbuf = NULL;		
  }	
}
