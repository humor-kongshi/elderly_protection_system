#ifndef __DHT11_H
#define __DHT11_H 
#include <stm32l1xx.h>
#include "delay.h"
////IO操作函数											   
//#define	DHT11_DQ_OUT GPIO_SetBits(GPIOB,GPIO_Pin_8) //数据端口	PB8
#define	DHT11_DQ_IN  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_8)  //数据端口	PB8

u8 DHT11_Init(void);//初始化DHT11
u8 DHT11_Read_Data(u8 *temp,u8 *humi);//读取温湿度
u8 DHT11_Read_Byte(void);//读出一个字节
u8 DHT11_Read_Bit(void);//读出一个位
u8 DHT11_Check(void);//检测是否存在DHT11
void DHT11_Rst(void);//复位DHT11    
void DHT11_IO_IN(void);
void delay_us(uint32_t value);
void DHT11_Rst(void)   ;
#endif















