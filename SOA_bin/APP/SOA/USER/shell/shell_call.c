#include "stm32f1xx_hal.h"
#include "os.h"
#include "stdio.h"
#include "string.h"
#include "shell.h"
#include "log.h"
#include "shell_call.h"
#include "com_shell.h"
#include "string.h"
#include "service_shell.h"
#include "data_service.h"
#include "DA_service.h"
#include "register_service.h"
#include "net_service.h"
#include "hl_msgdeal.h"
#include "format.h"
#include "lwip.h"
#include "math.h"
#include "sds.h"
#include "heartbeat_service.h"
#include "tcp_service.h"


eExecStatus shell_clear_process( int ac,
                                signed char *av[],
                                signed char **ppcStringReply)
{
    shell_printf("\033[0;0H\033[2J");

    return SHELL_EXECSTATUS_OK;
}

 
eExecStatus shell_log_process( int ac,
                                signed char *av[],
                                signed char **ppcStringReply)
{
    if(0 == strcmp((char *)av[0], "on"))
    {
        log_level_set((uint8_t)~LOG_LEVEL_OFF);
    }
    else if(0 == strcmp((char *)av[0], "off"))
    {
        log_level_set(LOG_LEVEL_OFF);
    }
    else
    {
        return SHELL_EXECSTATUS_KO;
    }
    return SHELL_EXECSTATUS_OK;
}



eExecStatus shell_set_process( int ac,
                                    signed char *av[],
                                    signed char **ppcStringReply)
{
    eExecStatus exec_status = SHELL_EXECSTATUS_OK;
    //心跳间隔
    if(0 == strcmp((char *)av[0], "heartbeat"))
    {
      uint8_t str_len = 0;
      uint16_t set_interval = 0;
      str_len = strlen((char *)av[1]);
      if(str_len > 4)
      {
        shell_printf("Set heartbeat fail\r\n");
        return SHELL_EXECSTATUS_KO;
      }
      set_interval = atoi((char *)av[1]);
      set_config_report(0 , set_interval);
      if(MsgDeal_Send(NULL, get_heartbeat_service_handler(), MSG_ID_HEARTBEAT_DURATION_CHANGE, NULL, 0) != 0)
      {
        logerr("shell service send to heartbeat service fail !\r\n");
      }
      if(MsgDeal_Send(NULL, get_tcp_service_handler(), MSG_ID_HEARTBEAT_DURATION_CHANGE, NULL, 0) != 0)
      {
        logerr("data service send to tcp service fail !\r\n");
      }
      shell_printf("Set heartbeat ok\r\n");
    }
    //本地IP
    else if(0 == strcmp((char *)av[0], "ip"))
    {
      uint8_t ip[4] = {0};
      uint8_t netmask[4] = {0};
      uint8_t gateway[4] = {0};
      uint8_t i = 0;
      uint8_t ip_setCounter = 0;
      char* ipdata;
      if((char *)av[1] != NULL)
      {
        ipdata = strtok((char *)av[1], ".");
        ip[0] = atoi(ipdata);
        i++;
        while( ipdata != NULL && i<4 )
        {
          ipdata = strtok(NULL, ".");
          ip[i] = atoi(ipdata);
          i++;
        }
        if(i == 4)
        {
          ip_setCounter++;
        }
        i = 0;
      }
      if((char *)av[2] != NULL)
      {
        ipdata = strtok((char *)av[2], ".");
        netmask[0] = atoi(ipdata);
        i++;
        while( ipdata != NULL && i<4 )
        {
          ipdata = strtok(NULL, ".");
          netmask[i] = atoi(ipdata);
          i++;
        }
        if(i == 4)
        {
          ip_setCounter++;
        }
        i = 0;
      }
      if((char *)av[3] != NULL)
      {
        ipdata = strtok((char *)av[3], ".");
        gateway[0] = atoi(ipdata);
        i++;
        while( ipdata != NULL && i<4 )
        {
          ipdata = strtok(NULL, ".");
          gateway[i] = atoi(ipdata);
          i++;
        }
        if(i == 4)
        {
          ip_setCounter++;
        }
        i = 0;
      }
      if(ip_setCounter == 3)
      {
        set_lwip_local_para(ip ,netmask ,gateway);
        SDS_Poll();
        shell_printf("Set ip ok\r\n");
        if(MsgDeal_Send(NULL, get_net_service_handler(), MSG_ID_NET_IP_CHANGE, NULL, 0) != 0)
        {
          shell_printf("shell service send to tcp service ip change fail !\r\n");
        }
      }
      else
      {
        shell_printf("ip set error\r\n");
      }
    }
    //服务器IP
    else if(0 == strcmp((char *)av[0], "server"))
    {
      uint8_t server_ip[4] = {0};
      uint16_t port;
      uint8_t server_setCounter = 0;
      uint8_t i = 0;
      char* server_ipdata;
      if((char *)av[1] != NULL)
      {
        server_ipdata = strtok((char *)av[1], ".");
        server_ip[0] = atoi(server_ipdata);
        i++;
        while( server_ipdata != NULL && i<4 )
        {
          server_ipdata = strtok(NULL, ".");
          server_ip[i] = atoi(server_ipdata);
          i++;
        }
        if(i == 4)
        {
          server_setCounter++;
        }
        i = 0;
      }
      if((char *)av[2] != NULL)
      {
        port = atoi((char *)av[2]);
        server_setCounter++;
      }
      if(server_setCounter == 2 )
      {
        set_lwip_remote_para(server_ip, port);
        SDS_Poll();
        if(MsgDeal_Send(NULL, get_net_service_handler(), MSG_ID_NET_REMOTE_IP_CHANGE, NULL, 0) != 0)
        {
          shell_printf("shell service send to tcp service remote ip change fail !\r\n");
        }
        shell_printf("shell set server succcess !\r\n");
      }
      else
      {
        shell_printf("shell set server error !\r\n");
      }
    }
    //子设备 
    else if(0 == strcmp((char *)av[0], "subdev"))
    {
      uint8_t sensor_Data[7] = {0};
      uint8_t sensor_DataLen = 0;
      uint8_t errorFlag = RESET;
      
      if(0 == strcmp((char *)av[1], "deleteAll"))
      {
        sensor_Data[0] = 0;
        sensor_DataLen++;
      }
      else if(0 == strcmp((char *)av[1], "set"))
      {
        sensor_Data[0] = 1;
        sensor_DataLen++;
      }
      else if(0 == strcmp((char *)av[1], "delete"))
      {
        sensor_Data[0] = 2;
        sensor_DataLen++;
      }
      else
      {
        errorFlag = SET;
      }
      for(uint8_t i=2; i<7; i+=2)
      {
        if((char *)av[i] != NULL && (char *)av[i+1] != NULL)
        {
          if(0 == strcmp((char *)av[i], "meteo"))
          {
            sensor_Data[sensor_DataLen] = 0;
            sensor_DataLen++;
            if((char *)av[i+1] != NULL)
            {
              sensor_Data[sensor_DataLen] = atoi((char *)av[i+1]);
              if(sensor_Data[sensor_DataLen] == 0)
              {
                errorFlag = SET;
                break;
              }
              sensor_DataLen++;
            }
            else
            {
              errorFlag = SET;
              break;
            }
          }
          else if(0 == strcmp((char *)av[i], "gas"))
          {
            sensor_Data[sensor_DataLen] = 1;
            sensor_DataLen++;
            if((char *)av[i+1] != NULL)
            {
              sensor_Data[sensor_DataLen] = atoi((char *)av[i+1]);
              if(sensor_Data[sensor_DataLen] == 0)
              {
                errorFlag = SET;
                break;
              }
              sensor_DataLen++;
            }
            else
            {
              errorFlag = SET;
              break;
            }
          }
          else if(0 == strcmp((char *)av[i], "water"))
          {
            sensor_Data[sensor_DataLen] = 2;
            sensor_DataLen++;
            if((char *)av[i+1] != NULL)
            {
              sensor_Data[sensor_DataLen] = atoi((char *)av[i+1]);
              if(sensor_Data[sensor_DataLen] == 0)
              {
                errorFlag = SET;
                break;
              }
              sensor_DataLen++;
            }
            else
            {
              errorFlag = SET;
              break;
            }
          }
          else if(0 == strcmp((char *)av[i], "multi"))
          {
            sensor_Data[sensor_DataLen] = 3;
            sensor_DataLen++;
            if((char *)av[i+1] != NULL)
            {
              sensor_Data[sensor_DataLen] = atoi((char *)av[i+1]);
              if(sensor_Data[sensor_DataLen] == 0)
              {
                errorFlag = SET;
                break;
              }
              sensor_DataLen++;
            }
            else
            {
              errorFlag = SET;
              break;
            }
          }
        }
      }
      if(sensor_DataLen == 0)
      {
        errorFlag = SET;
      }
      if(sensor_Data[0] >0 )
      {
        if(sensor_DataLen <= 1)
        {
          errorFlag = SET;
        }
      }
      if(errorFlag == RESET)
      {
        if(0 == set_sensor(sensor_Data, sensor_DataLen))
        {
          SDS_Poll();
          shell_printf("shell set subdev ok !\r\n");
        }
        else
        {
          shell_printf("shell set subdev error 1!\r\n");
        }
      }
      else
      {
        shell_printf("shell set subdev error !\r\n");
      }
    }
    //报警 
    else if(0 == strcmp((char *)av[0], "alarm"))
    {
      uint8_t alarm_Data[6] = {0};
      uint8_t alarm_counter = 0;
      
      if(0 == strcmp((char *)av[1], "water"))
      {
        alarm_Data[0] = 1;
        alarm_counter++;
      }
      if((char *)av[2] != NULL)
      {
        uint16_t alarm_value;
        alarm_value =  atoi((char *)av[2]);
        //if(alarm_value != 0)
        {
          alarm_Data[1] = LO_UINT16(alarm_value);
          alarm_Data[2] = HI_UINT16(alarm_value);  
          alarm_counter++;
        }
      }
      if((char *)av[3] != NULL)
      {
        uint16_t recover_value;
        recover_value =  atoi((char *)av[3]);
        //if(recover_value != 0)
        {
          alarm_Data[3] = LO_UINT16(recover_value);
          alarm_Data[4] = HI_UINT16(recover_value);  
          alarm_counter++;
        }
      }
      if(0 == strcmp((char *)av[4], "enable"))
      {
        alarm_Data[5] = 1;
        alarm_counter++;
      }
      else if(0 == strcmp((char *)av[4], "disable"))
      {
        alarm_Data[5] = 0;
        alarm_counter++;
      }
      if(alarm_counter == 4 )
      {
        if(0 == set_alarm(alarm_Data, 6))
        {
          SDS_Poll();
          shell_printf("shell set alarm ok !\r\n");
        }
        else
        {
          shell_printf("shell set alarm error1 !\r\n");
        }
      }
      else  
      {
        shell_printf("shell set alarm error !\r\n");
      }
    }
    else if(0 == strcmp((char *)av[0], "deviceid"))
    {
      uint16_t mseq = 0;
      uint8_t deviceid_counter = 0;
      SysConf_t config = {0};
      get_config(&config);
      
      if(0 == strcmp((char *)av[1], "clear"))
      {
        clear_devId();
        shell_printf("Set devid ok\r\n");
        return exec_status;
      }
      mseq = atoi((char *)av[1]);
      if(mseq < 4096)
      {
        deviceid_counter++;
      }
      if(config.device_id_setFlag == SET)
      {
        shell_printf("devid already set\r\n");
        deviceid_counter = 0;
      }
      if(deviceid_counter == 1)
      {
        uint8_t addr[6] = {0};
        addr[0] = 0x00;
        addr[1] = 0x01;
        addr[2] = 0x13;
        addr[3] = 0x81;
        addr[4] = HI_UINT16(mseq);
        addr[5] = LO_UINT16(mseq);
        set_config_addr(addr);
        shell_printf("Set deviceid ok\r\n");
        SysConf_t config = {0};
        get_config(&config);
        shell_printf("Device ID: 02-01-%02x-%02x-%02x-%02x-%02x-%02x\r\n", config.dev_addr[0], config.dev_addr[1], config.dev_addr[2], config.dev_addr[3], config.dev_addr[4], config.dev_addr[5]);
      }
      else
      {
        shell_printf("Set deviceid fail\r\n");
      }
    }
    //其他参数
    else
    {
      shell_printf("set error !\r\n");
    }
    return exec_status;
}

void show_devinfo(void)
{
  SysConf_t config = {0};
  get_config(&config); 
  uint8_t soft_ver[4] = {0};
  
  soft_ver[0] = config.soft_version & 0x000000FF;
  soft_ver[1] = (config.soft_version >> 8 ) & 0x000000FF;
  soft_ver[2] = (config.soft_version >> 16 ) & 0x000000FF;
  soft_ver[3] = (config.soft_version >> 24 ) & 0x000000FF;
  char* revision = NULL; 
  revision = Revision.Revision;
  
  shell_printf("Hardware version: %d.%d.%d.%d\r\n", config.hard_version[0], config.hard_version[1], config.hard_version[2], config.hard_version[3]);
  shell_printf("Software version: %d.%d.%d", soft_ver[2], soft_ver[1], soft_ver[0]);
  shell_printf("%s\r\n", revision);
  shell_printf("Device ID: 02-01-%02x-%02x-%02x-%02x-%02x-%02x\r\n", config.dev_addr[0], config.dev_addr[1], config.dev_addr[2], config.dev_addr[3], config.dev_addr[4], config.dev_addr[5]);
}

void show_heartbeat(void)
{
  SysConf_t config = {0};
  get_config(&config);
  shell_printf("heartbeat: %d s\r\n", config.report_interval);
}

void show_ip(void)
{
  lwip_dev_t lwip;
  get_lwip_dev(&lwip);
  shell_printf("Local ip: %d.%d.%d.%d\r\n", lwip.ip[0], lwip.ip[1], lwip.ip[2], lwip.ip[3]);
  shell_printf("Gate way: %d.%d.%d.%d\r\n", lwip.gateway[0], lwip.gateway[1], lwip.gateway[2], lwip.gateway[3]);
  shell_printf("Net mask: %d.%d.%d.%d\r\n", lwip.netmask[0], lwip.netmask[1], lwip.netmask[2], lwip.netmask[3]);
  
  shell_printf("Server ip: %d.%d.%d.%d\r\n", lwip.remoteip[0], lwip.remoteip[1], lwip.remoteip[2], lwip.remoteip[3]);
  shell_printf("Port: %d\r\n", lwip.remoteport);
}

void show_subdev(void)
{
  SysConf_t config = {0};
  get_config(&config);
  
  shell_printf("subdev meteo:%d  enable:%d\r\n", config.Sensor[0].addr, config.Sensor[0].enFlag);
  shell_printf("subdev gas:%d  enable:%d\r\n", config.Sensor[1].addr, config.Sensor[1].enFlag);
  shell_printf("subdev multi:%d  enable:%d\r\n", config.Sensor[3].addr, config.Sensor[3].enFlag);
}

void show_alarm(void)
{
  SysConf_t config = {0};
  get_config(&config);
  shell_printf("water: \r\nalarmValue:%dCM \r\nrecoverValue:%dCM enable:%d\r\n", config.Alarm[0].alarm_value, config.Alarm[0].recover_value, config.Alarm[0].enFlag);
}

void show_devstatus(void)
{
  uint8_t device_state;
  device_state = get_device_state();
  if(device_state == DEVICE_STATE_NOT_RDY)
  {
    shell_printf("DEVICE_STATE_NOT_RDY\r\n");
  }
  else if(device_state == DEVICE_STATE_STAND_BY)
  {
    shell_printf("DEVICE_STATE_STAND_BY\r\n");
  }
  else if(device_state == DEVICE_STATE_WORK)
  {
    shell_printf("DEVICE_STATE_WORK\r\n");
  }
  else if(device_state == DEVICE_STATE_UPGRADE)
  {
    shell_printf("DEVICE_STATE_UPGRADE\r\n");
  }
}


eExecStatus shell_show_process( int ac,
                                signed char *av[],
                                signed char **ppcStringReply)
{
  if(0 == strcmp((char *)av[0], "all"))
  {
    show_devinfo();
    show_heartbeat();
    show_ip();
    show_subdev();
    //show_alarm();
  }
  else if(0 == strcmp((char *)av[0], "devinfo"))
  {
    show_devinfo();
  }
  else if(0 == strcmp((char *)av[0], "heartbeat"))
  {
    show_heartbeat();
  }
  else if(0 == strcmp((char *)av[0], "ip"))
  {
    show_ip();
  }
  else if(0 == strcmp((char *)av[0], "subdev"))
  {
    show_subdev();
  }
  else if(0 == strcmp((char *)av[0], "alarm"))
  {
    show_alarm();
  }
  else if(0 == strcmp((char *)av[0], "devstatus"))
  {
    show_devstatus();
  }
  else
  {
    shell_printf("show error !\r\n");
  }
  return SHELL_EXECSTATUS_OK;
}

eExecStatus shell_event_process( int ac, 
                          signed char *av[],
                          signed char **ppcStringReply)
{
  
  if(0 == strcmp((char *)av[0], "alarm"))
  {
    SysConf_t config = {0};
    get_config(&config);
    uint8_t DA_startFlag = RESET;
    for(uint8_t i=0; i<4; i++)
    {
      if(config.Sensor[i].type == 2)
      {
        shell_printf("Water -> alarm enable:%d\r\n value:%d\r\n", config.Sensor_State[i].eventFlag, config.Sensor_State[i].value);
        DA_startFlag = SET;
      }
    }
    if(DA_startFlag == RESET)
    {
      shell_printf("DA_service not start\r\n");
    }
  } 
  else if(0 == strcmp((char *)av[0], "fault"))
  {
    SysConf_t config = {0};
    Sensor_State_t State = {0};
    get_config(&config);
    for(uint8_t i=0; i<4; i++)
    {
      if(config.Sensor[i].type == 0)
      {
        get_sensor_State(&State, 0);
        shell_printf("Meteo -> error enable:%d\r\n", State.errorFlag);
      }
      else if(config.Sensor[i].type == 1)
      {
        get_sensor_State(&State, 1);
        shell_printf("gas -> error enable:%d\r\n", State.errorFlag);
      }
      else if(config.Sensor[i].type == 2)
      {
        get_sensor_State(&State, 2);
        shell_printf("water -> error enable:%d\r\n", State.errorFlag);
      }
    }
  }
  else
  {
    shell_printf("event error !\r\n");
  }
  return SHELL_EXECSTATUS_OK;
}

eExecStatus shell_ctrl_process( int ac, 
                          signed char *av[],
                          signed char **ppcStringReply)
{
  if(0 == strcmp((char *)av[0], "reboot"))
  {
    shell_printf("reboot success\r\n");
    uint8_t log_cause;
    log_cause = OUT_CAUSE_SYS_RESET;
    if(MsgDeal_Send(NULL, get_register_service_handler(), MSG_ID_LOG_OUT, &log_cause, 1) != 0)
    {
      shell_printf("shell send to register service fail !\r\n");
    }
  }
  else if(0 == strcmp((char *)av[0], "reset"))
  {
    factory_resrt();
    shell_printf("reset success\r\n");
  }
  else
  {
    shell_printf("ctrl error !\r\n");
  }
  return SHELL_EXECSTATUS_OK;
}

eExecStatus shell_hb_process( int ac, 
                          signed char *av[],
                          signed char **ppcStringReply)
{
  if(0 == strcmp((char *)av[0], "status"))
  {
     Monitor_Data Monitor = {0};
     get_Monitor(&Monitor);
     shell_printf("Dm: %d\r\nSm: %d\r\nTa: %d\r\nUa: %d\r\nPa: %d\r\nRai: %d\r\nRad: %d\r\nUit: %d\r\nNS: %d\r\nPM2: %d\r\nPM10: %d\r\n",Monitor.meteor.Dm, Monitor.meteor.Sm, Monitor.meteor.Ta, Monitor.meteor.Ua, Monitor.meteor.Pa, Monitor.meteor.Rai, Monitor.meteor.Uit, Monitor.meteor.NS, Monitor.meteor.PM2, Monitor.meteor.PM10);
     shell_printf("CO:%d\r\nSO2:%d\r\nH2S:%d\r\nNO2:%d\r\nO3:%d\r\nNO:%d\r\n", Monitor.gas.CO, Monitor.gas.SO2, Monitor.gas.H2S, Monitor.gas.NO2, Monitor.gas.O3, Monitor.gas.NO);
     shell_printf("Water:%d\r\n", Monitor.elec_leak.Water);
     shell_printf("Light:%d\r\n", Monitor.light);
  }
  else
  {
    shell_printf("hb error !\r\n");
  }
  return SHELL_EXECSTATUS_OK;
}