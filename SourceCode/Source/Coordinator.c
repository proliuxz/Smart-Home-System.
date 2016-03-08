#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "Common.h"
#include "DebugTrace.h"

#if!defined(WIN32)
 #include "OnBoard.h"
#endif

/* Hal */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#include "OSAL_Nv.h"

const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS]=
{
  GENERICAPP_CLUSTERID
};

const SimpleDescriptionFormat_t GenericApp_SimpleDesc =
{
  GENERICAPP_ENDPOINT,
  GENERICAPP_PROFID,
  GENERICAPP_DEVICEID,
  GENERICAPP_DEVICE_VERSION,
  GENERICAPP_FLAGS,
  GENERICAPP_MAX_CLUSTERS,
  (cId_t*)GenericApp_ClusterList,
  0,
  (cId_t *)NULL
};

  static uint8 SerialApp_TxLen;
  endPointDesc_t GenericApp_epDesc;
  byte GenericApp_TaskID;
  byte GenericApp_TransID; 
  static void rxCB(uint8 port,uint8 event);
  
  void GenericApp_MessageMSGCB(afIncomingMSGPacket_t *pkt);
  void GenericApp_SendTheMessage(unsigned char theMessageData);
  
  void GenericApp_Init(byte task_id) 
  {
    halUARTCfg_t uartConfig;
    GenericApp_TaskID=task_id;
    GenericApp_TransID=0;
    
    GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;
    GenericApp_epDesc.task_id = &GenericApp_TaskID;
    GenericApp_epDesc.simpleDesc
              =(SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
    GenericApp_epDesc.latencyReq = noLatencyReqs;
    afRegister(&GenericApp_epDesc);
    
    uartConfig.configured = TRUE;
    uartConfig.baudRate = HAL_UART_BR_115200;
    uartConfig.flowControl = FALSE;
	uartConfig.flowControlThreshold=64;
	uartConfig.rx.maxBufSize=128;
	uartConfig.tx.maxBufSize=128;
	uartConfig.idleTimeout=6;
	uartConfig.intEnable=TRUE;
    uartConfig.callBackFunc = rxCB;

    HalUARTOpen(0,&uartConfig);
  }
  
  UINT16 GenericApp_ProcessEvent(byte task_id,UINT16 events)
  {
   afIncomingMSGPacket_t *MSGpkt;
  if(events & SYS_EVENT_MSG)
  {
    MSGpkt=(afIncomingMSGPacket_t *)osal_msg_receive(GenericApp_TaskID);
    while(MSGpkt)
    {
      switch(MSGpkt->hdr.event)
      {
      case AF_INCOMING_MSG_CMD:
        GenericApp_MessageMSGCB(MSGpkt);
        break;
        
      default:
        break;
      }
      osal_msg_deallocate((uint8 *)MSGpkt);
      MSGpkt=(afIncomingMSGPacket_t *)osal_msg_receive(GenericApp_TaskID);
    }
    return(events ^ SYS_EVENT_MSG);
  }
    return 0;
  }
  
static void rxCB(uint8 port,uint8 enent)
{
  unsigned char Uartbuf[80];
  if((enent&(HAL_UART_RX_FULL|HAL_UART_RX_ABOUT_FULL|HAL_UART_RX_TIMEOUT))&&
#if SERIAL_APP_LOOPBACK
    (SerialApp_Txlen<SERIAL_APP_TX_MAX))
#else 
    !SerialApp_TxLen)
#endif
    {
        SerialApp_TxLen=HalUARTRead(0,Uartbuf,80);
	if(SerialApp_TxLen)
	{
	HalUARTWrite(0,Uartbuf,1);
	if(Uartbuf[0]=='0')
	GenericApp_SendTheMessage('1');
	else if(Uartbuf[0]=='1')
	GenericApp_SendTheMessage('0');
        SerialApp_TxLen=0;
	}
    }
}
void GenericApp_SendTheMessage(unsigned char theMessageData)
{
afAddrType_t my_DstAddr;
my_DstAddr.addrMode=(afAddrMode_t)Addr16Bit;
my_DstAddr.endPoint=GENERICAPP_ENDPOINT;
my_DstAddr.addr.shortAddr=0xFFFF;

AF_DataRequest(&my_DstAddr
,&GenericApp_epDesc
,GENERICAPP_CLUSTERID
,1
,&theMessageData
,&GenericApp_TransID
,AF_DISCV_ROUTE
,AF_DEFAULT_RADIUS);
}

 void GenericApp_MessageMSGCB(afIncomingMSGPacket_t *pkt)
  {
    unsigned char buffer[12];
    switch(pkt->clusterId)
    {
     case GENERICAPP_CLUSTERID:
      osal_memcpy(buffer,pkt->cmd.Data,12);
      unsigned char buffer2[3];
      unsigned char buffer3[2];
      buffer2[0]=buffer[8];
      buffer2[1]='.';
      buffer2[2]=buffer[9];
      HalUARTWrite(0,"SB板编号：",10);
      HalUARTWrite(0,buffer,4);
      HalUARTWrite(0,"\n",1);
      if(buffer[4]=='1')
      {
      HalUARTWrite(0,"LED灯:开",8);
      HalUARTWrite(0,"--",2);
      }
      else
      {
      HalUARTWrite(0,"LED灯:关",8);
      HalUARTWrite(0,"--",2);
      }
     
     if(buffer[5]=='1')
      {
      HalUARTWrite(0,"光电传感器:暗色",15);
     HalUARTWrite(0,"--",2);
      }
      else
      {
      HalUARTWrite(0,"光电传感器:亮色",15);
      HalUARTWrite(0,"--",2);
      }
       if(buffer[6]=='1')
      {
      HalUARTWrite(0,"电磁继电器:常开闭合",19);
      HalUARTWrite(0,"--",2);
      }
      else
      {
      HalUARTWrite(0,"电磁继电器:常闭闭合",19);
      HalUARTWrite(0,"--",2);
      }
       if(buffer[7]=='0')
      {
      HalUARTWrite(0,"火焰传感器:无火焰",17);
      }
      else
      {
      HalUARTWrite(0,"火焰传感器:有火焰",17);
      }
      HalUARTWrite(0,"--",2);
      HalUARTWrite(0,"光敏电压:",9);
      HalUARTWrite(0,buffer2,3);
      HalUARTWrite(0,buffer3,2);
      HalUARTWrite(0,"\n",1);
      HalUARTWrite(0,"\n",1);
      break;
    }
  }