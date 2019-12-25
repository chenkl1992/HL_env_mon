#define _HL_PACKET_C_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "hl_packet.h"


void WrLog(const char *pFmt, ...)
{
#if 0
    va_list ap;
    int     iRc = 0;

    va_start(ap, pFmt);
    iRc = vfprintf(stdout, pFmt, ap);
    va_end(ap);
    fflush(stdout);
#endif
    return;
}

void WrBin(const char *title, byte *buf, int len)
{
#if 0
    int line=0, rest=0;
    int i;

    assert((len>=0) && (len<=2048));
    line = len/16;
    rest = len%16;

    printf("[APPBIN] %s len:%d,line:%d,rest:%d\n", title, len, line, rest);

    for (i=0; i<line; i++)
    {
        printf("%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x Hex %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
            buf[i*16+ 0], buf[i*16+ 1], buf[i*16+ 2], buf[i*16+ 3],
            buf[i*16+ 4], buf[i*16+ 5], buf[i*16+ 6], buf[i*16+ 7],
            buf[i*16+ 8], buf[i*16+ 9], buf[i*16+10], buf[i*16+11],
            buf[i*16+12], buf[i*16+13], buf[i*16+14], buf[i*16+15]);
    }

    for (i=0; i<rest; i++)
    {
        (i == (rest-1))? printf("%02x", buf[line*16+ i]):printf("%02x-", buf[line*16+ i]);
    }

    (rest)? printf("\n[APPBIN] %s end\n", title): printf("[APPBIN] %s end\n", title);
#endif  
    return;
}

static void WrBin0(const char *title, byte *buf, int len)
{
#if 0
    int line=0, rest=0;
    int i;

    assert((len>=0) && (len<=2048));
    line = len/16;
    rest = len%16;

    printf("    [%s] len:%d,line:%d,rest:%d\n", title, len, line, rest);

    for (i=0; i<line; i++)
    {
        printf("    %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x Hex %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
            buf[i*16+ 0], buf[i*16+ 1], buf[i*16+ 2], buf[i*16+ 3],
            buf[i*16+ 4], buf[i*16+ 5], buf[i*16+ 6], buf[i*16+ 7],
            buf[i*16+ 8], buf[i*16+ 9], buf[i*16+10], buf[i*16+11],
            buf[i*16+12], buf[i*16+13], buf[i*16+14], buf[i*16+15]);
    }

    printf("    ");
    for (i=0; i<rest; i++)
    {
        (i == (rest-1))? printf("%02x", buf[line*16+ i]):printf("%02x-", buf[line*16+ i]);
    }

    if (rest) printf("\n");
#endif  
    return;
}

static int16u crc16_modbus(unsigned char *arr_buff, int len)
{
    unsigned short crc = 0xFFFF;
    int i, j;

    for (j = 0; j < len;j++)
    {
        crc = crc ^ arr_buff[j];
        for (i = 0; i<8; i++)
        {
            if ((crc & 0x0001) >0)
            {
                crc = crc >> 1;
                crc = crc ^ 0xa001;
            }
            else
                crc = crc >> 1;
        }
    }
    return (crc);
}

/*
 * Get length of variable-length array
 */
static int32s GetLengthOfVLA(ENUM_VAR_TYPE type, byte *pBuf, int32s len)
{
    byte   len8;
    int16u len16;
    
    switch (type)
    {
        case VAR_TYPE_BYTE:
            assert(1==len);
            memcpy(&len8, pBuf, len);
            return (int32s)len8;
        case VAR_TYPE_U16:
            assert(2==len);
            memcpy(&len16, pBuf, len);
            return (int32s)len16;
        case VAR_TYPE_IP_TYPE:
            assert(1==len);
            memcpy(&len8, pBuf, len);
            return (1==len8)?(4):(16);
        default:
            WrLog("error var type:[%d]", type);
            break;
    }
    
    assert(0);
    return (int32s)0;
}

/*
 * according to field config info, buffer is filled with struct
 * return: sum of size of all field, not size of struct  
 */
static int32s FillStruct2Buffer(byte *buf, void *st, FIELD_PARAM *field_param, int32s field_num)
{
    int16u t16;
    int32u t32;
    int32u pointer;
    int32s len = 0;
    int32s offset = 0;
    int32s i;
    
    assert((st!=NULL) && (field_param!=NULL) && (field_num>0) && (buf!=NULL));
    
    offset = 0;
    for (i=0; i<field_num; i++)
    {
        switch (field_param[i].type)
        {            
            case VAR_TYPE_U16:
                memcpy(&t16, (byte *)st+field_param[i].offset, field_param[i].len);
#if NET_BYTE_ORDER                
                t16  = htons(t16);
#endif
                memcpy(buf+offset, &t16, field_param[i].len);
                break;
            case VAR_TYPE_U32:
                memcpy(&t32, (byte *)st+field_param[i].offset, field_param[i].len);
#if NET_BYTE_ORDER
                t32  = htonl(t32);
#endif
                memcpy(buf+offset, &t32, field_param[i].len);
                break;
            case VAR_TYPE_VLA:
                len = GetLengthOfVLA(field_param[i-1].type, (byte *)st+field_param[i-1].offset, field_param[i-1].len);//get length of VLA via last field
                memcpy(buf+offset, (byte *)st+field_param[i].offset, len);
                field_param[i].len = len;
                break;
            case VAR_TYPE_POINTER:
                memcpy(&pointer, (byte *)st+field_param[i].offset, field_param[i].len);
                len = GetLengthOfVLA(field_param[i-1].type, (byte *)st+field_param[i-1].offset, field_param[i-1].len);
                memcpy(buf+offset, (void *)pointer, len);
                field_param[i].len = len;
                break;
            case VAR_TYPE_BYTE:
            case VAR_TYPE_FLA:
            case VAR_TYPE_IP_TYPE:
            case VAR_TYPE_IGNORE:
            default:
                memcpy(buf+offset, (byte *)st+field_param[i].offset, field_param[i].len);
                break;
        }
        offset += field_param[i].len;
    }
    
    return offset;
}


/*
 * according to field config info, struct is filled with buffer
 * return: sum of size of all field, not size of struct 
 */
static int32s FillBuffer2Struct(void *st, FIELD_PARAM *field_param, int32s field_num, byte *buf)
{
    int16u t16;
    int32u t32;
    int32s len = 0;
    int32s offset = 0;
    int32s i;
    
    assert((field_param!=NULL) && (field_num>0));
    
    offset = 0;
    for (i=0; i<field_num; i++)
    {
        switch (field_param[i].type)
        {            
            case VAR_TYPE_U16:
                memcpy(&t16, buf+offset, field_param[i].len);
#if NET_BYTE_ORDER                
                t16  = ntohs(t16);
#endif
                memcpy((byte *)st+field_param[i].offset, &t16, field_param[i].len);
                break;
            case VAR_TYPE_U32:
                memcpy(&t32, buf+offset, field_param[i].len);
#if NET_BYTE_ORDER
                t32  = ntohl(t32);
#endif
                memcpy((byte *)st+field_param[i].offset, &t32, field_param[i].len);
                break;
            case VAR_TYPE_VLA:
                len = GetLengthOfVLA(field_param[i-1].type, (byte *)st+field_param[i-1].offset, field_param[i-1].len);//get length of string via last field
                memcpy((byte *)st+field_param[i].offset, buf+offset, len);
                field_param[i].len = len;
                break;
            case VAR_TYPE_POINTER://todo
                break;
            case VAR_TYPE_BYTE:
            case VAR_TYPE_FLA:
            case VAR_TYPE_IP_TYPE:
            case VAR_TYPE_IGNORE:
            default:
                memcpy((byte *)st+field_param[i].offset, buf+offset, field_param[i].len);
                break;
        }
        offset += field_param[i].len;        
    }
    
    return offset;
}

/*
 * Get length of all fields of struct
 */
#if 0
static int32s GetFieldsLength(void *st, FIELD_PARAM *field_param, int32s field_num)
{
    int32s len = 0;
    int32s offset = 0;
    int32s i;
    
    assert((field_param!=NULL) && (field_num>0));
    
    offset = 0;
    for (i=0; i<field_num; i++)
    {
        if (st != NULL)
        {
            switch (field_param[i].type)
            {            
            case VAR_TYPE_VLA:
                len = GetLengthOfVLA(field_param[i-1].type, (byte *)st+field_param[i-1].offset, field_param[i-1].len);//get length of string via last field
                field_param[i].len = len;
                break;
            case VAR_TYPE_POINTER://todo
                break;
            case VAR_TYPE_U16:
            case VAR_TYPE_U32:
            case VAR_TYPE_BYTE:
            case VAR_TYPE_FLA:
            case VAR_TYPE_IP_TYPE:
            case VAR_TYPE_IGNORE:
            default:
                break;
            }
        }
        offset += field_param[i].len;        
    }
    
    return offset;
}

/*
 *  according to field config info, print all field info of struct
 */

	 
static void PrintStruct(const char *name, void *st, FIELD_PARAM *field_param, int32s field_num)
{
    int8u  t8;
    int16u t16;
    int32u t32;
    int32s len;
    int32s i;

    assert((field_param!=NULL) && (field_num>0));

    WrLog("[STRU %s] content is:\n", name);
    for (i=0; i<field_num; i++)
    {
        switch (field_param[i].type)
        {            
            case VAR_TYPE_U16:
                memcpy(&t16, (byte *)st+field_param[i].offset, field_param[i].len);
                WrLog("    [%s=0x%x]\n", field_param[i].name, t16);
                break;
            case VAR_TYPE_U32:
                memcpy(&t32, (byte *)st+field_param[i].offset, field_param[i].len);
                WrLog("    [%s=0x%x]\n", field_param[i].name, t32);
                break;
            case VAR_TYPE_IP_TYPE:
            case VAR_TYPE_BYTE:
                memcpy(&t8, (byte *)st+field_param[i].offset, field_param[i].len);
                WrLog("    [%s=0x%x]\n", field_param[i].name, t8);
                break;
            case VAR_TYPE_FLA:
                WrBin0(field_param[i].name, (byte *)st+field_param[i].offset, field_param[i].len);
                break;
            case VAR_TYPE_VLA:
                len = GetLengthOfVLA(field_param[i-1].type, (byte *)st+field_param[i-1].offset, field_param[i-1].len);//get length of string via last field
                WrBin0(field_param[i].name, (byte *)st+field_param[i].offset, len);
                break;
            case VAR_TYPE_IGNORE:
            default:
                WrBin0(field_param[i].name, (byte *)st+field_param[i].offset, field_param[i].len);
                break;
        }        
    }
    WrLog("[STRU %s] content end!\n", name);
}
#endif
/*
 * parse param item
 */
static int parse_param(PARAM_ITEM_S *param, byte *pbuf)
{
    int offset = 0;

    assert(pbuf != NULL);
    
    memcpy(&(param->pid),  pbuf+offset, sizeof(byte));
    offset++;
    memcpy(&(param->dlen), pbuf+offset, sizeof(byte));
    if (param->dlen > MAX_PARAM_LENGTH)
    {
        return -1;
    }
    offset++;
    memcpy(param->data, pbuf+offset, param->dlen);
    offset += param->dlen;

    return offset;
}

/*
 * parse param set
 */
int parse_paramset(PARAM_ITEM_S *param, byte *pbuf, int len)
{
    byte  param_num;
    int   offset = 0;
    int   i, plen;

    memcpy(&param_num, pbuf, sizeof(param_num));
    if (param_num > 7)
    {
        return -1;
    }
    offset++;
    for (i=0; i<param_num; i++)
    {
        plen = parse_param(&param[i], pbuf+offset);
        if (plen < 0)
        {
            return -2;
        }
        offset += plen;
    }

    if (offset != len)
    {
        return -3;
    }
        
    return param_num;
}

/*
 * pack param item
 */
static int pack_param(byte *pbuf, PARAM_ITEM_S *param)
{
    int offset = 0;
    
    memcpy(pbuf+offset, &param->pid, sizeof(param->pid));
    offset++;
    memcpy(pbuf+offset, &param->dlen, sizeof(param->dlen));
    offset++;
    memcpy(pbuf+offset, param->data, param->dlen);
    offset += param->dlen;
    
    return offset;
}

/*
 * pack param set
 */
int pack_paramset(byte *pbuf, PARAM_ITEM_S *param, int param_num)
{
    int offset = 0;
    int i;

    pbuf[offset++] = param_num;
    for (i=0; i<param_num; i++)
    {
        offset += pack_param(pbuf+offset, &param[i]);
        if (offset > 1024)
        {
            return -1;
        }
    }

    return offset;
}

void print_paramset(PARAM_ITEM_S *item, int param_num)
{
    int i;

    //WrLog("[paramset] content is:\n");
    printf("    param_num=%d\n", param_num);
    for (i=0; i<param_num; i++)
    {
        printf("    id=%d, sub_id=0x%02x, param_len=%d\n", i+1, item[i].pid, item[i].dlen);
        WrBin0("param_content", item[i].data, item[i].dlen);
    }
    WrLog("[paramset] content end\n");
}

/*
 * check if it's valid packet
 */
int is_packet(byte *pbuffer, int len)
{
    int16u blen;
    int16u crc16;

    assert((pbuffer!=NULL) && (len > 0));

    if (pbuffer[0] != g_head_flg)
    {
        return -1;
    }
    
    memcpy(&blen, &pbuffer[PKT_DLEN_OFFSET], 2);
    if (blen+PKT_FIX_LENGTH > len)
    {
        return -2;
    }

    if (pbuffer[blen+PKT_FIX_LENGTH-1] != g_tail_flg)
    {
        return -3;
    }
    
    crc16 = crc16_modbus(pbuffer+1, blen+18);
    if (0 != memcmp(&crc16, &pbuffer[blen+19], sizeof(crc16)))
    {
        return -4;
    }

    return 0;
}

/*
 * parse packet to head, param
 */
int parse_packet(PKT_HEAD_S *phead, PARAM_ITEM_S *param, byte *pbuf, int len)
{
    int  offset;

    assert((pbuf!=NULL) && (len > 0));

    offset = FillBuffer2Struct(phead, FP_MsgHead, ARR_NUM(FP_MsgHead), pbuf);
    if (phead->dlen > 1024)
    {
        return -1;
    }

    if (phead->dlen+PKT_FIX_LENGTH > len)
    {
        return -2;
    }

    if (0x80 != phead->main_cmd)
    {
        if ((0x01==phead->main_cmd) || (0x02==phead->main_cmd))
        {
            return parse_paramset(param, pbuf+offset, phead->dlen);
        }
        else
        {
            param[0].pid  = pbuf[offset];
            param[0].dlen = phead->dlen-1;
            memcpy(param[0].data, &pbuf[offset+1], param[0].dlen);
        
            return 1;
        }
    }
    else
    {
        if ((0x01==pbuf[offset]) || (0x02==pbuf[offset]))
        {
            return parse_paramset(param, pbuf+offset+1, phead->dlen-1);
        }
        else
        {
            param[0].pid = pbuf[offset+1];
            param[0].dlen= phead->dlen-2;
            memcpy(param[0].data, &pbuf[offset+2], param[0].dlen);
        
            return 1;
        }
    }
}

/*
 * build request packet based on head, param
 */
int pack_req_packet(byte *pbuf, PKT_HEAD_S *phead, PARAM_ITEM_S *param, int param_num)
{
    int offset;
    int16u crc16;
    
    if ((0x01==phead->main_cmd) || (0x02==phead->main_cmd))
    {
        phead->dlen = pack_paramset(pbuf+PKT_DATA_OFFSET, param, param_num);
    }
    else
    {
        memcpy(pbuf+PKT_DATA_OFFSET, &param[0].pid, 1);
        memcpy(pbuf+PKT_DATA_OFFSET+1, param[0].data, param[0].dlen);
        phead->dlen = param[0].dlen+1;
    }
    offset  = FillStruct2Buffer(pbuf, phead, FP_MsgHead, ARR_NUM(FP_MsgHead));
    offset += phead->dlen;
    
    crc16 = crc16_modbus(pbuf+1, phead->dlen+18);
    memcpy(pbuf+offset, &crc16, sizeof(crc16)); 
    offset += sizeof(crc16);
    memcpy(pbuf+offset, &g_tail_flg, sizeof(g_tail_flg));
    offset += sizeof(g_tail_flg);

    return offset;
}

/*
 * build ack packet based on head, param
 */
int pack_ack_packet(byte *pbuf, PKT_HEAD_S *phead, PARAM_ITEM_S *param, int param_num)
{
    int offset = 0;
    int16u crc16;
    
    if ((0x01==phead->main_cmd) || (0x02==phead->main_cmd))
    {
        pbuf[PKT_DATA_OFFSET] = phead->main_cmd; 
        phead->dlen = 1+pack_paramset(pbuf+PKT_DATA_OFFSET+1, param, param_num);
    }
    else
    {
        pbuf[PKT_DATA_OFFSET] = phead->main_cmd;
        memcpy(pbuf+PKT_DATA_OFFSET+1, &param[0].pid, 1);
        memcpy(pbuf+PKT_DATA_OFFSET+2, param[0].data, param[0].dlen);
        phead->dlen = 2+param[0].dlen;
    }
    phead->main_cmd = 0x80;
    offset = FillStruct2Buffer(pbuf, phead, FP_MsgHead, ARR_NUM(FP_MsgHead));
    offset += phead->dlen;
    
    crc16 = crc16_modbus(pbuf+1, phead->dlen+18);
    memcpy(pbuf+offset, &crc16, sizeof(crc16));
    offset += sizeof(crc16);
    memcpy(pbuf+offset, &g_tail_flg, sizeof(g_tail_flg));
    offset += sizeof(g_tail_flg);

    return offset;
}

