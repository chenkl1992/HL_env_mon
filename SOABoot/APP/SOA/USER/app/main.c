/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include "stm32f1xx_hal.h"
#include "printf-stdarg.h"
#include "sFlash.h"
#include "sds.h"
#include "crc.h"
#include "sds_table.h"
#include "assert.h"

#define BLOCK_SIZE								0x2000                  //8K
#define JUMP_TO_APP_FLAG                                                        0x12345678ul
#define APP_ADDR								0x08008800ul            //bootloader 32K + 2K
#define PAGE_SIZE								2048                    //2048
#define APP_INFO_AT								APP_ADDR - 64
/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
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

Img_head_t Img_head = {0};
typedef  void (*pFunction)(void);
__no_init static volatile uint32_t _jump_to_app;


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    assert(0);
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    //Error_Handler();
    assert(0);
  }
}

static void _JumpToApp(void)
{
  register pFunction Jump_To_Application;
  uint32_t JumpAddress;	
  if (((*(__IO uint32_t*)APP_ADDR) & 0x2FFF0000 ) == 0x20000000) //64K RAM判断
  { 
    /* Jump to user application */
    JumpAddress = *(__IO uint32_t*) (APP_ADDR + 4);
    Jump_To_Application = (pFunction) JumpAddress;
    /* Initialize user application's Stack Pointer */
    __set_MSP(*(__IO uint32_t*) APP_ADDR);
    Jump_To_Application();
  }
}

static void JumpToApp(void)
{
  _jump_to_app = JUMP_TO_APP_FLAG;
  __disable_irq();
  HAL_NVIC_SystemReset();	
}

void sysconfig_edit(void)
{
  SDS_Edit(&sysconfig);
}

void sysconfig_save(void)
{
  SDS_Save(&sysconfig);
}

void set_config_upgradeFlag(uint8_t upgrade)
{
  sysconfig_edit();
  sysconfig.upgradeState = upgrade;
  sysconfig_save();
  SDS_Poll();
}

static HAL_StatusTypeDef Flash_Erase(uint32_t erase_addr , uint8_t num_page )
{
  uint32_t Error;
  HAL_StatusTypeDef status;
  HAL_FLASH_Unlock();
  
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR | FLASH_FLAG_BSY);	
  
  FLASH_EraseInitTypeDef  FLASH_EraseInit;
  
  FLASH_EraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
  FLASH_EraseInit.Banks = FLASH_BANK_1;
  FLASH_EraseInit.PageAddress = erase_addr;
  FLASH_EraseInit.NbPages = num_page;
  status = HAL_FLASHEx_Erase(&FLASH_EraseInit, &Error);
  return status;
}

static uint8_t _app_info_is_ok(void)
{
  uint8_t crc8;
  sFlash_ReadBytes(0, &Img_head, sizeof(Img_head_t));
  crc8 = CRC8_CCITT_Calc(&Img_head, sizeof(Img_head_t)-1);
  if(crc8 == Img_head.crc8)
    return 1;
  else return 0;
}

static uint8_t _app_crc_is_ok(void)
{
  if(!_app_info_is_ok())
    return 0;
  uint32_t addr = 0;
  uint32_t crc_calc = 0xFFFFFFFF;
  uint32_t crc_in_code ;
  
  uint32_t Firmware_size = Img_head.flen;
  uint8_t Buf[BLOCK_SIZE] = {0};
  sFlash_ReadBytes(sizeof(Img_head_t) + Firmware_size, &crc_in_code, 4);
  
  uint16_t pack_num = (Firmware_size + sizeof(Img_head_t)) / BLOCK_SIZE;
  uint16_t cn_len  = (Firmware_size + sizeof(Img_head_t)) % BLOCK_SIZE;
  
  for(int i =0; i < pack_num ; i++)
  {
    sFlash_ReadBytes(addr, Buf, BLOCK_SIZE);
    crc_calc = CRC32_IEEE_Calc2(Buf, BLOCK_SIZE, crc_calc);
    addr += BLOCK_SIZE;
  }
  sFlash_ReadBytes(addr, Buf, cn_len);
  crc_calc = CRC32_IEEE_Calc2(Buf, cn_len, crc_calc);
  if(crc_calc == crc_in_code)
    return 1;
  else return 0;
}

static uint8_t copy_flash_to_app(void)
{
  uint32_t app_addr = APP_INFO_AT;
  uint32_t addr = 0;
  uint32_t Firmware_size = 0;
  Firmware_size = Img_head.flen;
  
  uint32_t rbuf[BLOCK_SIZE/sizeof(uint32_t)] = {0};
  uint16_t pack_num = (Firmware_size + 64 + 4) / BLOCK_SIZE;
  uint16_t cn_len  = (Firmware_size + 64 + 4) % BLOCK_SIZE;	
  
  uint16_t pages = 1 + (Firmware_size + 4) / PAGE_SIZE;
  if((Firmware_size + 4) % PAGE_SIZE != 0)  
    pages++;
  
  printf("Firmware_size:%d\r\n", Firmware_size);
  printf("pages:%d\r\n", pages);
  Flash_Erase(app_addr, pages);
  HAL_FLASH_Unlock();
  for(int i =0; i < pack_num ; i++)
  {
    sFlash_ReadBytes(addr, rbuf, BLOCK_SIZE);
    for (uint32_t i = 0; i< BLOCK_SIZE/4; i++)
    {
      if (*(volatile uint32_t *)(app_addr) != rbuf[i])
      {
        HAL_StatusTypeDef  status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, app_addr, rbuf[i]);
        if (status != HAL_OK)
        {
          HAL_FLASH_Lock();
          return 0;
        }
      }
      app_addr += 4;
    }
    addr += BLOCK_SIZE;
  }
  if(cn_len != 0)
  {
    memset(rbuf,0xFF,sizeof(rbuf));
    sFlash_ReadBytes(addr, rbuf, cn_len);
    uint32_t wpage = cn_len / 4 ;
    if(cn_len % 4 != 0)
      wpage++;
    
    for (uint32_t i = 0; i < wpage; i++)
    {
      if (*(volatile uint32_t *)(app_addr) != rbuf[i])
      {
        HAL_StatusTypeDef  status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, app_addr, rbuf[i]);
        if (status != HAL_OK)
        {
          HAL_FLASH_Lock();
          return 0;
        }
      }
      app_addr += 4;
    }
  }
  /*已经拷贝完成*/
  HAL_FLASH_Lock();
  return 1;
}

void User_EventProcess(void)
{
  switch(sysconfig.upgradeState)
  {
  case NO_UPDAGRADE:
    JumpToApp();
    break;
  case FINISH_UPDAGRADE:
    JumpToApp();
    break;   
  case NEED_UPDAGRADE:
    if(_app_crc_is_ok())
    {
      //copy
      if(copy_flash_to_app())
      {
        set_config_upgradeFlag(FINISH_UPDAGRADE);
        printf("copy_success\r\n");
        JumpToApp();
      }
      else
      {
        printf("copy_error\r\n");
        HAL_Delay(5000);
      }
    }
    else
    {
      set_config_upgradeFlag(SELF_TEST_FAIL);
      JumpToApp();
      printf("Self test fail\r\n");
    }
    break;
    
  default:
    break;
  }
}

void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
}

int  main (void)
{
  if (_jump_to_app == JUMP_TO_APP_FLAG)
  {
    _jump_to_app = 0;
    _JumpToApp();
  }
#warning 升级  
    HAL_Init();                                                 /* Initialize STM32Cube HAL Library*/
    SystemClock_Config();
    MX_GPIO_Init();
    
    debug_init();
    printf("boot start\r\n");
    sFlash_Init();
    SDS_Init();
    printf("upgradeState: %d\r\n", sysconfig.upgradeState);
    while(1)
    {
      User_EventProcess();
    }
}

