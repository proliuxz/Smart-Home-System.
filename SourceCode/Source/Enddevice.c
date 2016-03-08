#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "Common.h"
#include "DebugTrace.h"
#include "ioCC2530.h"
#include <string.h>
#include "gm.h"
#define LED2 P1_2
#define ECHO P1_3
#define ALARM P2_0
#define uchar unsigned char
#define uint unsigned int

#define SEND_DATA_EVENT 0x01

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"

#define SEND_DATA_EVENT 0x01

const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS] =
{
  GENERICAPP_CLUSTERID
};
void SetMessage(unsigned char temp[],int n);

const SimpleDescriptionFormat_t GenericApp_SimpleDesc =
{
  GENERICAPP_ENDPOINT,              //  int Endpoint;
  GENERICAPP_PROFID,                //  uint16 AppProfId[2];
  GENERICAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  GENERICAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  GENERICAPP_FLAGS,                 //  int   AppFlags:4;
  
  0,
  (cId_t*)NULL,
  GENERICAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)GenericApp_ClusterList,  //  byte *pAppInClusterList;
};


endPointDesc_t GenericApp_epDesc;
byte GenericApp_TaskID;   // Task ID for internal task/event processing
byte GenericApp_TransID; 
devStates_t GenericApp_NwkState;
void SampleApp_HandleKeys(uint8 shift,uint8 keys);
void GenericApp_MessageMSGCB(afIncomingMSGPacket_t *MSGpkt);
void GenericApp_SendTheMessage(void);

void GenericApp_Init( byte task_id )
{
  halUARTCfg_t uartConfig;
  GenericApp_TaskID = task_id;
  GenericApp_NwkState = DEV_INIT;
  GenericApp_TransID = 0;

  GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_epDesc.task_id = &GenericApp_TaskID;
  GenericApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
            
  GenericApp_epDesc.latencyReq = noLatencyReqs;
  afRegister( &GenericApp_epDesc );
}


UINT16 GenericApp_ProcessEvent( byte task_id, UINT16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case AF_INCOMING_MSG_CMD:
        GenericApp_MessageMSGCB(MSGpkt);
          break;
        case ZDO_STATE_CHANGE:
          GenericApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if (GenericApp_NwkState == DEV_END_DEVICE)
          {
            osal_set_event(GenericApp_TaskID,SEND_DATA_EVENT);
          }
          break;

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
    }
    return (events ^ SYS_EVENT_MSG);
  }
  if ( events & SEND_DATA_EVENT )
  {
    GenericApp_SendTheMessage();
    osal_start_timerEx( GenericApp_TaskID,SEND_DATA_EVENT,1000);
    return (events^SEND_DATA_EVENT);
  }
  return 0;
}

void GenericApp_MessageMSGCB(afIncomingMSGPacket_t *pkt)
{
  unsigned char buffer[10];
  switch(pkt->clusterId)
  {
  case GENERICAPP_CLUSTERID:
  osal_memcpy(&buffer,pkt->cmd.Data,10);
  if(buffer[0]=='1')
  P1_0=1;
  if(buffer[0]=='0')
  P1_0=0;
  break;
  }
}

void GenericApp_SendTheMessage( void )
{
  unsigned char theMessageData[11] = "SB01000000";
  SetMessage(theMessageData,11);
  afAddrType_t my_DstAddr;
  my_DstAddr.addrMode=(afAddrMode_t)Addr16Bit;
  my_DstAddr.endPoint=GENERICAPP_ENDPOINT;
  my_DstAddr.addr.shortAddr=0xFFFF;
  AF_DataRequest(&my_DstAddr
  ,&GenericApp_epDesc
  ,GENERICAPP_CLUSTERID
  ,osal_strlen("SB01000000")+1
  ,theMessageData
  ,&GenericApp_TransID
  ,AF_DISCV_ROUTE
  ,AF_DEFAULT_RADIUS);
}

void SetMessage(unsigned char temp[],int n)
{ 
  P2DIR |= 0x01;
  P1DIR |= 0x07;
  P0SEL &=0x20;
  ALARM =0;
  LED2=1;
    if(P0_7)
    {
    	temp[4]='1';
        LED2=0;
    }
    if(!(P0&0x02))
      {
        temp[5]='1';
      }
    if(!(P1_0))
    {
      temp[6]='1';
    }
    if(P0_5)
    {
     temp[7]='1';
     ALARM= 1;
    }
  else  if(!P0_5)
    {
      ALARM =0;
}
  temp[8]=(uchar)getVoltage();
  temp[9]=temp[8]%10+0x30;
  temp[8]=temp[8]/10+0x30;
}