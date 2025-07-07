#include "led.h"
#include "delay.h"
void LED_Init(void)
{		
    GPIO_InitTypeDef   GPIO_InitStructure;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);	 	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //���츴�����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    //����
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOC,GPIO_Pin_8);       //��Ƭ��LED�ƿ��� pc8 pc9
    GPIO_ResetBits(GPIOC,GPIO_Pin_9);
    GPIO_SetBits(GPIOC,GPIO_Pin_7);         //POWKEYEN PC7�ߵ�ƽ����POWKEY�������� ����Ϊ�ߵ�ƽ
    GPIO_ResetBits(GPIOC,GPIO_Pin_2);       //PSM���� �ߵ�ƽ����PSM���� ����Ϊ�͵�ƽ
    GPIO_ResetBits(GPIOC,GPIO_Pin_6);       //ģ�鸴λ PC6���Ƹߵ�ƽģ�鸴λ �͵�ƽ��������
    //GPIO_SetBits(GPIOC,GPIO_Pin_1);       	//PC1���Ƽ̵��������͹ر� �͵�ƽ����
    GPIO_SetBits(GPIOC,GPIO_Pin_4);         //PC4����485�ջ��߷���Ĭ�ϸߵ�ƽ����Ϊ����

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);     //Enable GPIOA clocks
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //���츴�����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    //����
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
   GPIO_SetBits(GPIOC,GPIO_Pin_6);       //ģ�鸴λ PC6���Ƹߵ�ƽģ�鸴λ �͵�ƽ��������
   delay_ms(500);
   GPIO_ResetBits(GPIOC,GPIO_Pin_6);       //ģ�鸴λ PC6���Ƹߵ�ƽģ�鸴λ �͵�ƽ��������
   delay_ms(300);

}





