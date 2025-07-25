#include "dht11.h"
//设置输入
void DHT11_IO_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    /*Configure GPIO pin : PB8 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
}
//设置输出
void DHT11_IO_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
}

//复位DHT11
void DHT11_Rst(void)           
{                 
    DHT11_IO_OUT();         //SET OUTPUT
    GPIO_ResetBits(GPIOB,GPIO_Pin_8);                 //设置DQ为0
    delay_ms(20);            //延时18ms以上
    GPIO_SetBits(GPIOB,GPIO_Pin_8);                   //DQ=1
    delay_us(30);             //延时20~40us
}
//检查DHT11是否存在 1不存在，0存在
uint8_t DHT11_Check(void)            
{   
    uint8_t retry=0;
    DHT11_IO_IN();//SET INPUT
    while (DHT11_DQ_IN&&retry<100)//DHT11延时40~80us
    {
        retry++;
        delay_us(3);// 1us
    };
    if(retry>=100)return 1;
    else retry=0;
    while (!DHT11_DQ_IN&&retry<100)//DHT11延时40~80us
    {
        retry++;
        delay_us(3);
    };
    if(retry>=100)return 1;
    return 0;
}
//读取单个位数据
uint8_t DHT11_Read_Bit(void)                          
{
    uint8_t retry=0;
    while (DHT11_DQ_IN&&retry<100)//DHT11延时40~80us
    {
        retry++;
        delay_us(3);
    }
    retry=0;
    while (!DHT11_DQ_IN&&retry<100)//DHT11延时40~80us
    {
        retry++;
        delay_us(3);
    }
    delay_us(50);//??40us
    if(DHT11_DQ_IN)
        return 1;
    else
        return 0;
}
//读取字节数据值

uint8_t DHT11_Read_Byte(void)    
{        
    uint8_t i,dat;
    dat=0;
    for (i=0;i<8;i++)
    {
        dat<<=1;
        dat|=DHT11_Read_Bit();
    }
    return dat;
}
//读取温湿度数据值
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi)    
{        
    uint8_t buf[5];
    uint8_t i;
    DHT11_Rst();
    if(DHT11_Check()==0)
    {
        for(i=0;i<5;i++)//读取40bit数据
        {
            buf[i]=DHT11_Read_Byte();
        }
        if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
        {
            *humi=buf[0];
            *temp=buf[2];
        }
    }else return 1;
    return 0;
}
//初始化DHT11温湿度传感器           
uint8_t DHT11_Init(void)
{          
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
    DHT11_Rst();  //复位HT11
    return DHT11_Check();//查看DHT11的返回
}

