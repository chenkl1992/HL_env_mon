#ifndef _HL_PACKET_H_
#define _HL_PACKET_H_

typedef int             int32s;
typedef unsigned int    int32u;
typedef short           int16s;
typedef unsigned short  int16u;
typedef char            int8s;
typedef unsigned char   int8u;
typedef unsigned char   byte;

#ifndef NULL
#define NULL            (void*)(0)
#endif

#define MEM_OFFSET(s, f)  ((size_t)&((s *)0)->f)            //offset of field in struct
#define MEM_SIZEOF(s, f)  ((size_t)sizeof(((s *)0)->f))     //length of field in struct
#define FIELD_CFG(s, f)   MEM_OFFSET(s, f), MEM_SIZEOF(s, f), #f //field config:offset, length, name
#define ARR_NUM(A)        (sizeof(A)/sizeof((A)[0]))        //num of array

typedef enum
{
    VAR_TYPE_BYTE,
    VAR_TYPE_U16,
    VAR_TYPE_U32,
    VAR_TYPE_FLA,   //fixed length array
    VAR_TYPE_VLA,    //variable length array
    VAR_TYPE_IP_TYPE,
    VAR_TYPE_POINTER,
    VAR_TYPE_IGNORE
} ENUM_VAR_TYPE;

typedef struct
{
    ENUM_VAR_TYPE   type;
    int32s          offset;
    int32s          len;
    const char     *name;
} FIELD_PARAM;

#define MAX_PARAM_LENGTH  64
typedef struct
{
    byte pid;
    byte dlen;
    byte data[MAX_PARAM_LENGTH];
} PARAM_ITEM_S;

#define PKT_DATA_SIZE   1024
#define PKT_DLEN_OFFSET 17
#define PKT_DATA_OFFSET (PKT_DLEN_OFFSET+2) 
#define PKT_FIX_LENGTH  22

__packed typedef struct
{
    byte   head_flg;
    byte   dev_addr[8];
    byte   sign_flg;
    byte   reserved[4];
    int16u seq_no;
    byte   main_cmd;
    int16u dlen;
}PKT_HEAD_S;

typedef struct
{
    byte   data[PKT_DATA_SIZE]; 
}PKT_BODY_S;

typedef struct
{
    byte   sign[64];
    int16u crc16;
    byte   tail_flg;
}PKT_TAIL_S;

#ifdef _HL_PACKET_C_

const byte g_head_flg = 0x68;
const byte g_tail_flg = 0x16;

FIELD_PARAM FP_MsgHead[]=
{
    {VAR_TYPE_BYTE,    FIELD_CFG(PKT_HEAD_S, head_flg)},
    {VAR_TYPE_FLA,     FIELD_CFG(PKT_HEAD_S, dev_addr)},
    {VAR_TYPE_BYTE,    FIELD_CFG(PKT_HEAD_S, sign_flg)},
    {VAR_TYPE_FLA,     FIELD_CFG(PKT_HEAD_S, reserved)},
    {VAR_TYPE_U16,     FIELD_CFG(PKT_HEAD_S, seq_no)},
    {VAR_TYPE_BYTE,    FIELD_CFG(PKT_HEAD_S, main_cmd)},
    {VAR_TYPE_U16,     FIELD_CFG(PKT_HEAD_S, dlen)},
};

FIELD_PARAM FP_MsgBody[]=
{
    {VAR_TYPE_VLA,     FIELD_CFG(PKT_BODY_S, data)},
};

FIELD_PARAM FP_MsgTail[]=
{
    {VAR_TYPE_BYTE,    FIELD_CFG(PKT_TAIL_S, sign)},    
    {VAR_TYPE_U16,     FIELD_CFG(PKT_TAIL_S, crc16)},
    {VAR_TYPE_BYTE,    FIELD_CFG(PKT_TAIL_S, tail_flg)},    
};

#endif


extern int is_packet(byte *pbuffer, int len);
extern int parse_packet(PKT_HEAD_S *phead, PARAM_ITEM_S *param, byte *pbuf, int len);
extern int pack_req_packet(byte *pbuf, PKT_HEAD_S *phead, PARAM_ITEM_S *param, int param_num);
extern int pack_ack_packet(byte *pbuf, PKT_HEAD_S *phead, PARAM_ITEM_S *param, int param_num);
#endif
