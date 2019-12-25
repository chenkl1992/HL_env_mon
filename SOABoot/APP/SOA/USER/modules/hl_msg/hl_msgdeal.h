#ifndef __HL_MSGDEAL_H__
#define __HL_MSGDEAL_H__
#include "lib_mem.h"
#include "os.h"

typedef enum
{
  MSG_ID_NULL,
  MSG_ID_LOG_IN,
  MSG_ID_LOG_OUT,
  MSG_ID_LOG_IN_ACK,
  MSG_ID_LOG_IN_TIMEOUT,
  MSG_ID_START_HEARTBEAT,
  MSG_ID_HEARTBEAT_DURATION_CHANGE,
  MSG_ID_HEARTBEAT_TIMEOUT,
  MSG_ID_UDP_PACK,
  MSG_ID_TCP_PACK,
  MSG_ID_NET_IP_CHANGE,
  MSG_ID_NET_REMOTE_IP_CHANGE,
  MSG_ID_DA_DATARCV,
  MSG_ID_DA_DATA_TIMEOUT,
  MSG_ID_EVENT_ALARM,
  MSG_ID_EVENT_FAULT,
  MSG_ID_EVENT_ACK,
  MSG_ID_EVENT_STOP,
  MSG_ID_EVENT_TIMEOUT,
  MSG_ID_UPGRADE_IND,
  MSG_ID_POLICY,
  MSG_ID_CTRL,
}MSG_ID_TYPE;

typedef OS_Q	*MessageAddr;

typedef struct
{
	MSG_ID_TYPE 	mtype;
	MessageAddr	srcID;
	MessageAddr     dstID;
	unsigned int 	reserved;
	unsigned int 	len;
	void*		buf;
}MSG_BUF_t;

void MsgDeal_Init(void);
int  MsgDeal_Send(MessageAddr srcID, MessageAddr dstID, MSG_ID_TYPE msgid, void *buf, int len);
int  MsgDeal_SendExt(MessageAddr dstID, MSG_BUF_t *msg);
int  MsgDeal_Recv(MessageAddr dstID, MSG_BUF_t **MsgInfo , int timeout);
void MsgDeal_Free(MSG_BUF_t *MsgInfo);
void MsgDeal_Clr(MessageAddr addr);

#endif