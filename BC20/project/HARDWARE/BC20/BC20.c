///AT+NSOCL=0，由于关闭指定的socket
#include "BC20.h"
#include "main.h"
#include "string.h"
#include "oled.h"
char *strx,*extstrx;
char atstr[BUFLEN];
int err;    //全局变量
char atbuf[BUFLEN];
char objtnum[BUFLEN];//观察号
char distnum[BUFLEN];//观察号
BC20 BC20_Status;
int  errcount=0;	//发送命令失败次数 防止死循环

#define PRODUCEKEY "k25fqNEHgIr"        //阿里云产品密钥
#define DEVICENAME "BC20GPSTRACK"           //阿里云设备名称
#define DEVICESECRET "b3febda31df42c5f97fd19d345e8e87f" //阿里云设备密钥

char GPRMCSTR[128]; //转载GPS信息 GPRMC 经纬度存储的字符串
char GPRMCSTRLON[64]; //经度存储字符串
char GPRMCSTRLAT[64]; //维度存储字符串
char IMEINUMBER[64];//+CGSN: "869523052178994"


//////////////////下面是纠正火星坐标的变量定义/////////////////////////
int Get_GPSdata(void);
void Getdata_Change(char status);

    typedef struct 
{
char UtcDate[6];
char longitude[11];//经度原数据
char Latitude[10];//纬度源数据
char longitudess[4];//整数部分
char Latitudess[3];
char longitudedd[8];//小数点部分
char Latitudedd[8];
char Truelongitude[12];//转换过数据
char TrueLatitude[11];//转换过数据
char getstautus;//获取到定位的标志状态	
float gpsdata[2];
}LongLatidata;
LongLatidata latdata;
float tempdata[2];
char latStrAF[64];          //存放数据经纬度用来发送
char lonStrAF[64];   //存放数据经纬度用来显示
//////////////////////////火星纠偏结束///////////////////////////////////


void Clear_Buffer(void)//清空串口2缓存
{
    printf(buf_uart2.buf);  //清空前打印信息
    Delay(300);
    buf_uart2.index=0;
    memset(buf_uart2.buf,0,BUFLEN);
}

int BC20_Init(void)
{
    int errcount = 0;
    err = 0;    //判断模块卡是否就绪最重要
    printf("start init BC20\r\n");
    Uart2_SendStr("AT\r\n");
    Delay(300);
    printf(buf_uart2.buf);      //打印收到的串口信息
    printf("get back BC20\r\n");
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返回OK
    Clear_Buffer();	
    while(strx==NULL)
    {
        printf("\r\n单片机正在连接到模块...\r\n");
        Clear_Buffer();	
        Uart2_SendStr("AT\r\n");
        Delay(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返回OK
    }
    
    Uart2_SendStr("AT+CIMI\r\n");//获取卡号
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"ERROR");//只要卡不错误 基本就成功
    if(strx==NULL)
    {
        printf("我的卡号是 : %s \r\n",&buf_uart2.buf[8]);
        Clear_Buffer();	
        Delay(300);
    }
    else
    {
        err = 1;
        printf("卡错误 : %s \r\n",buf_uart2.buf);
        Clear_Buffer();
        Delay(300);
    }

    Uart2_SendStr("AT+CGATT=1\r\n");//激活网络，PDP，将移动终端连接到包域服务
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返OK
    Clear_Buffer();	
    if(strx)
    {
        Clear_Buffer();	
        printf("init PDP OK\r\n");
        Delay(300);
    }
    Uart2_SendStr("AT+CGATT?\r\n");//查询激活状态
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//返1 表明激活成功 获取到IP地址了
    Clear_Buffer();	
    errcount = 0;
    while(strx==NULL)
    {
        errcount++;
        Clear_Buffer();	
        Uart2_SendStr("AT+CGATT?\r\n");//获取激活状态
        Delay(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//返回1,表明注网成功
        if(errcount>100)     //防止死循环
        {
            err=1;
            errcount = 0;
            break;
        }
    }


    Uart2_SendStr("AT+QBAND?\r\n"); //允许错误值
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返回OK
    if(strx)
    {
        printf("========BAND========= \r\n %s \r\n",buf_uart2.buf);
        Clear_Buffer();
        Delay(300);
    }

    Uart2_SendStr("AT+CSQ\r\n");//查看获取CSQ值，判断信号强度
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+CSQ");//返回CSQ
    if(strx)
    {
        printf("信号质量:%s\r\n",buf_uart2.buf);
        Clear_Buffer();
        Delay(300);
    }
    Uart2_SendStr("AT+QCCID\r\n");
    Delay(300);
    Clear_Buffer();	

    return err;
}

void BC20_PDPACT(void)//激活场景，为连接服务器做准备
{
    int errcount = 0;
    Uart2_SendStr("AT+CGPADDR=1\r\n");//激活场景，返回设备IP
    Delay(300);
    Clear_Buffer();
    Uart2_SendStr("AT+CGSN=1\r\n");//激活场景，返回IMEI序列号和相关信息
    Delay(300);
    Clear_Buffer();
    Uart2_SendStr("AT+CGATT?\r\n");//激活场景，指示激活状态
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//注册上网络信息
    Clear_Buffer();	
    while(strx==NULL)
    {
        errcount++;
        Clear_Buffer();
        Uart2_SendStr("AT+CGATT?\r\n");//激活场景
        Delay(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//一定要终端入网
        if(errcount>100)     //防止死循环
        {
            errcount = 0;
            break;
        }
    }
    Delay(300);
    Clear_Buffer();	
    Delay(300);
}


void BC20_INITGNSS(void)//启动GPS
{
    int errcount = 0;
    Uart2_SendStr("AT+QGNSSC=1\r\n");//激活GPS 要等待很久启动GNSS
    Delay(1000);Delay(1000);Delay(1000);
    Clear_Buffer();
    Uart2_SendStr("AT+QGNSSC?\r\n");//查询GPS激活情况
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QGNSSC: 1");//启动成功
    Clear_Buffer();
    while(strx==NULL)
    {
        errcount++;
        Clear_Buffer();
        Uart2_SendStr("AT+QGNSSC?\r\n");//查询
        Delay(1000);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QGNSSC: 1");//启动成功
        if(errcount>100)     //防止死循环
        {
            errcount = 0;
            break;
        }
    }
    Clear_Buffer();
    Uart2_SendStr("AT+QGNSSRD=\"NMEA/RMC\"\r\n");
    Delay(300);
    Clear_Buffer();
}


/*********************************************************************************
AT+QGNSSRD="NMEA/RMC"

+QGNSSRD: $GNRMC,,V,,,,,,,,,,N,V*37

OK
***********************************************************************************/

char *Get_GPS_RMC(char type)
{
		Clear_Buffer();	
		memset(GPRMCSTR,0,128);
		Uart2_SendStr("AT+QGNSSRD=\"NMEA/RMC\"\r\n");//查询激活状态
		delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)"$GNRMC");//返1
		
		while(strx==NULL)
		{
				Clear_Buffer();	
				Uart2_SendStr("AT+QGNSSRD=\"NMEA/RMC\"\r\n");//获取激活状态
				delay_ms(300);
				strx=strstr((const char*)buf_uart2.buf,(const char*)"$GNRMC");//返回1,表明注网成功
		}
		sprintf(GPRMCSTR,"%s",strx);
	

		OLED_ShowString(0,2,"START GPS [OK] ");
		Clear_Buffer();	//打印收到的GPS信息
		GPRMCSTR[2]=	'P';
		
		printf("============GETGPRMC==============\r\n%s",GPRMCSTR);		//打印GPRMC
		if(GPRMCSTR[17]=='A')
		{
			memset(latStrAF,0,64);
			memset(lonStrAF,0,64);
			Get_GPSdata();
			
			if(type==1)
				return latStrAF;
			if(type==2)
				return lonStrAF;
			
		}
		return 0;
}



/*****************************************************
下面是矫正火星坐标的
AT+QGNSSRD="NMEA/RMC"

+QGNSSRD: $GNRMC,091900.00,A,2603.9680,N,11912.4174,E,0.189,,201022,,,A,V*1A

OK
*****************************************************/


//解GPS析函数
// $GNRMC,091900.00,A,2603.9680,N,11912.4174,E,0.189,,201022,,,A,V*1A
int Get_GPSdata()
{
		int i=0;
    strx=strstr((const char*)GPRMCSTR,(const char*)"A,");//获取纬度的位置
       if(strx)
        {
            for(i=0;i<9;i++)
            {
             latdata.Latitude[i]=strx[i+2];//获取纬度值2603.9576
            }
						strx=strstr((const char*)GPRMCSTR,(const char*)"N,");//获取经度值
						if(strx)
						{
								 for(i=0;i<10;i++)	//获取经度 11912.4098
								 {
										latdata.longitude[i]=strx[i+2];
								 }
								 
						}  
						
						printf("latdata.Latitude ,%s \r\n",latdata.Latitude);
						printf("latdata.longitude ,%s \r\n",latdata.longitude);
            latdata.getstautus=1;       
	    }
                            
		else
		{
						
				latdata.getstautus=0;
		 }
			Getdata_Change(latdata.getstautus);//数据换算
			Clear_Buffer();
		 return 0;

}

/*************解析出经纬度数据,然后直接提交数据*******************/	

void Getdata_Change(char status)
{
	unsigned char i;	
    	
    if(status)
    {

        for(i=0;i<3;i++)
						latdata.longitudess[i]=latdata.longitude[i];
				for(i=3;i<10;i++)
						latdata.longitudedd[i-3]=latdata.longitude[i];
			
			 latdata.gpsdata[0]=(latdata.longitudess[0]-0x30)*100+(latdata.longitudess[1]-0x30)*10+(latdata.longitudess[2]-0x30)\
		     +((latdata.longitudedd[0]-0x30)*10+(latdata.longitudedd[1]-0x30)+(float)(latdata.longitudedd[3]-0x30)/10+\
		     (float)(latdata.longitudedd[4]-0x30)/100+(float)(latdata.longitudedd[5]-0x30)/1000+(float)(latdata.longitudedd[6]-0x30)/10000)/60.0;//获取完整的数据
       
///////////////////////////////////////////
				for(i=0;i<2;i++)
						latdata.Latitudess[i]=latdata.Latitude[i];
				for(i=2;i<9;i++)
						latdata.Latitudedd[i-2]=latdata.Latitude[i];	
				 
			latdata.gpsdata[1]=(float)(latdata.Latitudess[0]-0x30)*10+(latdata.Latitudess[1]-0x30)\
		     +((latdata.Latitudedd[0]-0x30)*10+(latdata.Latitudedd[1]-0x30)+(float)(latdata.Latitudedd[3]-0x30)/10+\
		     (float)(latdata.Latitudedd[4]-0x30)/100+(float)(latdata.Latitudedd[5]-0x30)/1000+(float)(latdata.Latitudedd[6]-0x30)/10000)/60.0;//获取完整的数据b

	
				 sprintf(latStrAF,"%f",latdata.gpsdata[1]);
				 sprintf(lonStrAF,"%f",latdata.gpsdata[0]);
				 
				 			 
				 printf("latStrAF,%s \r\n",latStrAF);
				 printf("lonStrAF,%s \r\n",lonStrAF);
				 
    }
    else
    {
        latdata.gpsdata[0]=0;
        latdata.gpsdata[1]=0;
    }
		
	
}


int val = 100;
void BC20_Senddata(uint8_t *len,uint8_t *data)
{
    int errcount=0;
    memset(atstr,0,BUFLEN);
    sprintf(atstr,"AT+MIPLNOTIFY=0,%s,3303,0,5700,4,%s,%d,0,0\r\n",objtnum,len,val);
    Uart2_SendStr(atstr);//发送0 socketIP和端口后面跟对应数据长度以及数据
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返回OK
    while(strx==NULL)
    {
        errcount++;
        strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返回OK
        if(errcount>100)     //防止死循环
        {
            errcount = 0;
            break;
        }
    }

    Clear_Buffer();	
}


void BC20_RegALIYUNIOT(void)//平台注册
{
    u8  BC20_IMEI[20],i;//IMEI值
    int errcount = 0;
    Uart2_SendStr("AT+QMTDISC=0\r\n");//Disconnect a client from MQTT server
    delay_ms(300);
    Clear_Buffer();

    Uart2_SendStr("AT+QMTCLOSE=0\r\n");//删除句柄
    delay_ms(300);
    Clear_Buffer();

    memset(atstr,0,BUFLEN);
    sprintf(atstr,"AT+QMTCFG=\"ALIAUTH\",0,\"%s\",\"%s\",\"%s\"\r\n",PRODUCEKEY,DEVICENAME,DEVICESECRET);
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);//发送阿里云配置参数
    delay_ms(300);  //等待300ms反馈OK
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返OK
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//返OK
    }
    Clear_Buffer();

    Uart2_SendStr("AT+QMTOPEN=0,\"iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883\r\n");//登录阿里云平台
    delay_ms(3000);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTOPEN: 0,0");//返+QMTOPEN: 0,0
    while(strx==NULL)
    {
        errcount++;
        delay_ms(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTOPEN: 0,0");//返回OK
        if(errcount>500)     //防止死循环
        {
            GPIO_SetBits(GPIOC,GPIO_Pin_6);		//模块重启
            Delay(500);
            GPIO_ResetBits(GPIOC,GPIO_Pin_6);
            Delay(300);
            NVIC_SystemReset();
        }
    }
    Clear_Buffer();

    Uart2_SendStr("AT+CGSN=1\r\n");//获取模块的IMEI号
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGSN:");//返+CGSN:
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGSN:");//返+CGSN:
    }
    for(i=0;i<15;i++)
        BC20_IMEI[i]=strx[i+7];
    BC20_IMEI[15]=0;
    Clear_Buffer();

    printf("我的模块IMEI是:%s\r\n",BC20_IMEI);

    memset(atstr,0,BUFLEN);
    sprintf(atstr,"AT+QMTCONN=0,\"%s\"\r\n",DEVICENAME);//链接MQTT服务器
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);//发送链接到阿里云
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTCONN: 0,0,0");//返+QMTCONN: 0,0,0
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTCONN: 0,0,0");//返+QMTCONN: 0,0,0
    }
    Clear_Buffer();

    memset(atstr,0,BUFLEN);
    sprintf(atstr,"AT+QMTSUB=0,1,\"/%s/%s/user/get\",0 \r\n",PRODUCEKEY,DEVICENAME);
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);//发送链接到阿里云
    delay_ms(300);
    Clear_Buffer();
}


void BC20_ALYIOTSenddata2(u8 *len,u8 *data)//上发数据，上发的数据跟对应的插件有关系，用户需要注意插件然后对应数据即可
{
    memset(atstr,0,BUFLEN);
    sprintf(atstr,"AT+QMTPUB=0,1,1,0,\"/sys/%s/%s/thing/event/property/post\",\"{params:{LightLux:%s}}\"\r\n",PRODUCEKEY,DEVICENAME,data);
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTPUB: 0,1,0");//返SEND OK
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTPUB: 0,1,0");//返SEND OK
    }
    Clear_Buffer();
}


char send_json[BUFLEN]; //组建带数据的JSON
u8 Mqttaliyun_Savedata(u8 *t_payload,int temp,int humi,char *latstr,char *lonstr)
{

	  //发送json的总长度
    int json_len;
	  //发送的json格式
    char json[]="{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"CurrentTemperature\":{\"value\":%d},\"CurrentHumidity\":{\"value\":%d},\"GeoLocation\":{ \"value\":{\"Latitude\":%s,\"Longitude\":%s,\"Altitude\":1,\"CoordinateSystem\":1}}},\"method\":\"thing.event.property.post\"}";	 
		memset(send_json,0,BUFLEN);
    sprintf(send_json, json, temp, humi,latstr,lonstr); //组建需要发送的json 数据
    json_len = strlen(send_json)/sizeof(char);
  	memcpy(t_payload, send_json, json_len);
    return json_len;
}

void CSTX_4G_ALYIOTSenddataGPS(int temp,int humi,char *latstr,char *lonstr)//上发数据，上发的数据跟对应的插件有关系，用户需要注意插件然后对应数据即可
{

		memset(send_json,0,BUFLEN);
    memset(atstr,0,BUFLEN);
	
		Clear_Buffer();	//发送命令之前清空之前的模块反馈的数据
    sprintf(atstr,"AT+QMTPUB=0,0,0,0,\"/sys/%s/%s/thing/event/property/post\"\r\n",PRODUCEKEY,DEVICENAME); //先发送命令
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);//发送命令 模块发送命令
    delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
    while(strx==NULL)
    {
        errcount++;
        strx=strstr((const char*)buf_uart2.buf,(const char*)">");//模块反馈可以发送数据了
				delay_ms(30);
        if(errcount>100)     //防止死循环跳出
        {
            errcount = 0;
            break;
        }
    }
		
		Mqttaliyun_Savedata((u8*)send_json,temp,humi,latstr,lonstr);	//组建CJSON数据
		printf("send_json = %s \r\n",send_json);
		Uart2_SendStr((char *)send_json);	//发送完毕命令接下来就进行判断反馈
		delay_ms(30);
		UART2_send_byte(0x1A); //发送结束符
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTRECV: 0,0");//返发送成功
		errcount=0;
    while(strx==NULL)
    {
				errcount++;
				strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTRECV: 0,0");//返发送成功
				delay_ms(100);
				if(errcount>100)     //超时退出死循环 表示服务器连接失败
        {
            errcount = 0;
            break;
        }
    }
		delay_ms(300);
    Clear_Buffer();
}

void IWDG_Init(u8 prer,u16 rlr)
{
        IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //使能对寄存器IWDG_PR和IWDG_RLR的写操作

        IWDG_SetPrescaler(prer);  //设置IWDG预分频值:设置IWDG预分频值为64

        IWDG_SetReload(rlr);  //设置IWDG重装载值

        IWDG_ReloadCounter();  //按照IWDG重装载寄存器的值重装载IWDG计数器

        IWDG_Enable();  //使能IWDG
}
//喂独立看门狗
void IWDG_Feed(void)
{
        IWDG_ReloadCounter();//reload
}
