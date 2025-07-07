#include "main.h"
#include "usart.h"
#include "timer.h"
#include "delay.h"
#include "dht11.h"
#include "BC20.h"
#include "led.h"
#include "systempz.h"
#include "string.h"
#include "stdio.h"
#include "HEXSTR.h"
#include "oled.h"
#include "ADCx.h"
__IO uint16_t ADC_ConvertedValue;
//GPIO_InitTypeDef GPIO_InitStructure;
static __IO uint32_t TimingDelay;
extern int err;
extern int val;
u8 temp,humi;

int main(void)
{
    float adc1;
    char senddata[BUFLEN];
    char tempstr[BUFLEN];
		char *gpsStr; 							//GPS指向的位置
		char sendMutiData[BUFLEN]; 	//发送数据到服务
	  char gpsDatalat[64];
	  char gpsDatalon[64];
	
    if (SysTick_Config(SystemCoreClock / 1000))//设置24bit定时器 1ms中断一次
    {
        /* Capture error */
        while (1);
    }
    LED_Init();
    uart1_init(115200);
    uart2_init(115200);
    uart3_init(115200);
    delay_init();
    //NBIOT_RESET();
    LED_RUN();
    while(DHT11_Init());//DHT11
    OLED_Init();			//OLED
    OLED_Clear();
    OLED_ShowString(0,0,"STM32+BC20+WIFI");
    OLED_ShowString(0,2,"NBIOT GPS BOARD");

    ADC_DMA_Config();
    Uart1_SendStr("init stm32L COM1 \r\n"); //打印信息
    while(BC20_Init());
    BC20_PDPACT();
		BC20_INITGNSS();
    BC20_RegALIYUNIOT();
    while (1)
    {
			
				gpsStr = NULL;
				memset(sendMutiData,0,BUFLEN);//发送多传感器数据到服务器
				memset(gpsDatalat,0,64);//维度
				memset(gpsDatalon,0,64);//经度
				strcat(sendMutiData,"latitude=");//传递维度
				OLED_ShowString(0,4,"La:");//打印数据
				gpsStr=Get_GPS_RMC(1);//获取维度 这个是经过纠偏的纬度
				if(gpsStr)	//如果获取到了
				{
					strcat(sendMutiData,gpsStr);//传递维度
					strcat(gpsDatalat,gpsStr);//传递维度
					OLED_ShowString(27,4,(u8*) gpsStr);
				}
				else	//还没定位好
				{
					strcat(sendMutiData,"90");//传递维度
					strcat(gpsDatalat,"90");//传递维度
					OLED_ShowString(27,4,"           ");
					OLED_ShowString(27,4,"wrong");
				}
				
				strcat(sendMutiData,"&longitude="); //传递经度
				OLED_ShowString(0,6,"Lo:");//打印数据
				gpsStr=Get_GPS_RMC(2);//获取维度 这个是经过纠偏的经度
				if(gpsStr)	//获取到了经度
				{
					strcat(sendMutiData,gpsStr);//传递经度
					strcat(gpsDatalon,gpsStr);//传递经度
					OLED_ShowString(27,6,(u8*) gpsStr);
				}
				else	//还没定位好
				{
					strcat(sendMutiData,"0");//传递经度
					strcat(gpsDatalon,"0");//传递经度
					OLED_ShowString(27,6,"          ");
					OLED_ShowString(27,6,"wrong");
				}
				printf("sendMutiData=%s \r\n",sendMutiData);
				
        adc1=(float)((float)ADC_ConvertedValue*VREF)/MAX_CONVERTD; //PC0
        printf("adc1=%f \r\n",adc1);
				
        memset(tempstr,0,BUFLEN);
        memset(senddata,0,BUFLEN);
        sprintf(tempstr,"%.1f",adc1);
        strcat(senddata,tempstr);
        BC20_ALYIOTSenddata2("8",(u8 *)senddata);//发数据到平台端
				
				DHT11_Read_Data(&temp,&humi);//读取温湿度数据
        printf("温度：%d C \r\n",temp);
        printf("湿度：%d RH\r\n",humi);
				
				CSTX_4G_ALYIOTSenddataGPS(temp,humi,gpsDatalat,gpsDatalon);
				
        Delay(1000);
        GPIO_ToggleBits(GPIOC,GPIO_Pin_8);//翻转LED

    }
}


/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }
}
#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 

    while (1)
    {
    }
}
#endif

