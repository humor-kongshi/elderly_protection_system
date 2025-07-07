#include <stm32l1xx.h>
extern void GPIO_Configuration(void);
extern void RCC_Configuration(void);

extern void DELAY_100nS(unsigned int delaytime);
extern void DELAY_mS(unsigned int delaytime);
extern void DELAY_S(unsigned int delaytime);
extern void DELAY_M(unsigned int delaytime);

extern void SPI4W_WRITECOM(unsigned char INIT_COM);
extern void SPI4W_WRITEDATA(unsigned char INIT_DATA);
extern void MYRESET(void);
extern void WRITE_LUT(void);

extern void READBUSY(void);
extern void INIT_SSD1673(void);
extern void DIS_IMG(unsigned char num);

extern unsigned char qrcode[];
#define SDA_H  GPIO_SetBits(GPIOA, GPIO_Pin_7);	  
#define SDA_L  GPIO_ResetBits(GPIOA, GPIO_Pin_7);	  

#define SCLK_H GPIO_SetBits(GPIOA, GPIO_Pin_5);	  
#define SCLK_L GPIO_ResetBits(GPIOA, GPIO_Pin_5);

#define nCS_H  GPIO_SetBits(GPIOA, GPIO_Pin_4);	  
#define nCS_L  GPIO_ResetBits(GPIOA, GPIO_Pin_4);

#define nDC_H  GPIO_SetBits(GPIOB, GPIO_Pin_1); 
#define nDC_L  GPIO_ResetBits(GPIOB, GPIO_Pin_1);

#define nRST_H GPIO_SetBits(GPIOB, GPIO_Pin_0);  
#define nRST_L GPIO_ResetBits(GPIOB, GPIO_Pin_0);

#define nBUSY  GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_5)

#define DELAY_TIME	50    // ͼƬ��ʾ��ͣ��ʱ��(��λ:��)
// ����ͼ
#define PIC_WHITE                   255  // ȫ��
#define PIC_BLACK                   254  // ȫ��
#define PIC_Orientation             253  // ����ͼ
#define PIC_LEFT_BLACK_RIGHT_WHITE  249  // ����Ұ�
#define PIC_UP_BLACK_DOWN_WHITE     248  // �Ϻ��°�
#define BMPPIC							5 
