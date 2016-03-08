#include "ioCC2530.h"
#define uchar unsigned char
#define uint unsigned int
uchar getVoltage(void);
uchar getVoltage(void)
{   ADCCFG|=0x10;
    P0INP|=0x10;
  uint value_L,value_H;
  long avalue=0;
    value_H=value_L=0;
    ADCCON3|=0x84;
    ADCCON1|=0x40;
    while(!(ADCCON1&0x80));
    value_L=ADCL>>2;
    value_H|=ADCH;
    value_H=value_H<<6;
    avalue+=(value_H|value_L);    
    avalue=(int)(avalue);
    avalue=avalue*33/8191;
     return avalue;
}
