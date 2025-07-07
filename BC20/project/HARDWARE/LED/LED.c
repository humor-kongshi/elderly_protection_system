#include "led.h"
#include "delay.h"
void LED_Init(void)
{		
    GPIO_InitTypeDef   GPIO_InitStructure;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);	 	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    //上拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOC,GPIO_Pin_8);       //单片机LED灯控制 pc8 pc9
    GPIO_ResetBits(GPIOC,GPIO_Pin_9);
    GPIO_SetBits(GPIOC,GPIO_Pin_7);         //POWKEYEN PC7高电平控制POWKEY拉低启动 设置为高电平
    GPIO_ResetBits(GPIOC,GPIO_Pin_2);       //PSM唤醒 高电平控制PSM唤醒 设置为低电平
    GPIO_ResetBits(GPIOC,GPIO_Pin_6);       //模块复位 PC6控制高电平模块复位 低电平正常工作
    //GPIO_SetBits(GPIOC,GPIO_Pin_1);       	//PC1控制继电器开启和关闭 低电平开启
    GPIO_SetBits(GPIOC,GPIO_Pin_4);         //PC4控制485收或者发，默认高电平控制为发出

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);     //Enable GPIOA clocks
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    //上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOA,GPIO_Pin_8);


} 
void LED_RUN(void)
{
    //JDQOFF
    //delay_ms(500);
    LED1OFF;
    delay_ms(500);
    LED2OFF;
    delay_ms(500);
    LED3OFF;
}

void NBIOT_RESET(void)
{
   GPIO_SetBits(GPIOC,GPIO_Pin_6);       //模块复位 PC6控制高电平模块复位 低电平正常工作
   delay_ms(500);
   GPIO_ResetBits(GPIOC,GPIO_Pin_6);       //模块复位 PC6控制高电平模块复位 低电平正常工作
   delay_ms(300);

}





