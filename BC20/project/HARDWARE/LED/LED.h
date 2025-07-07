#include <stm32l1xx.h>
void LED_Init(void);
void LED_RUN(void);
void NBIOT_RESET(void);
#define LED1ON      GPIO_ResetBits(GPIOC,GPIO_Pin_8);
#define LED2ON      GPIO_ResetBits(GPIOC,GPIO_Pin_9);
#define LED3ON      GPIO_ResetBits(GPIOA,GPIO_Pin_8);
#define JDQON       GPIO_ResetBits(GPIOC,GPIO_Pin_1);
#define LED1OFF     GPIO_SetBits(GPIOC,GPIO_Pin_8);
#define LED2OFF     GPIO_SetBits(GPIOC,GPIO_Pin_9);
#define LED3OFF     GPIO_SetBits(GPIOA,GPIO_Pin_8);
#define JDQOFF      GPIO_SetBits(GPIOC,GPIO_Pin_1);




