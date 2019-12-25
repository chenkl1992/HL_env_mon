#ifndef __UPDATE_SERVICE_H__
#define __UPDATE_SERVICE_H__

#include  "os.h"

__packed typedef struct 
{
  char 		fileName[24];
  char 		buildTime[20];
  uint16_t	devType;
  uint32_t	softVersion;
  uint32_t	startAddr;
  uint32_t	flen;
  uint8_t	reserve[5];
  uint8_t	crc8;
}Img_head_t;

#define  NO_UPDAGRADE           0
#define  NEED_UPDAGRADE         1
#define  FINISH_UPDAGRADE       2
#define  SELF_TEST_FAIL         3
#define  RECOVER_SUCCESS        4
#define  RECOVER_FAIL           5

OS_Q*  get_update_service_handler(void);
OS_Q* get_update_data_handler(void);
void start_update_service(void);



#endif