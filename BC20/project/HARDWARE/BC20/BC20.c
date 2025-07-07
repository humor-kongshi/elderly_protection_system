///AT+NSOCL=0�����ڹر�ָ����socket
#include "BC20.h"
#include "main.h"
#include "string.h"
#include "oled.h"
char *strx,*extstrx;
char atstr[BUFLEN];
int err;    //ȫ�ֱ���
char atbuf[BUFLEN];
char objtnum[BUFLEN];//�۲��
char distnum[BUFLEN];//�۲��
BC20 BC20_Status;
int  errcount=0;	//��������ʧ�ܴ��� ��ֹ��ѭ��

#define PRODUCEKEY "k25fqNEHgIr"        //�����Ʋ�Ʒ��Կ
#define DEVICENAME "BC20GPSTRACK"           //�������豸����
#define DEVICESECRET "b3febda31df42c5f97fd19d345e8e87f" //�������豸��Կ

char GPRMCSTR[128]; //ת��GPS��Ϣ GPRMC ��γ�ȴ洢���ַ���
char GPRMCSTRLON[64]; //���ȴ洢�ַ���
char GPRMCSTRLAT[64]; //ά�ȴ洢�ַ���
char IMEINUMBER[64];//+CGSN: "869523052178994"


//////////////////�����Ǿ�����������ı�������/////////////////////////
int Get_GPSdata(void);
void Getdata_Change(char status);

    typedef struct 
{
char UtcDate[6];
char longitude[11];//����ԭ����
char Latitude[10];//γ��Դ����
char longitudess[4];//��������
char Latitudess[3];
char longitudedd[8];//С���㲿��
char Latitudedd[8];
char Truelongitude[12];//ת��������
char TrueLatitude[11];//ת��������
char getstautus;//��ȡ����λ�ı�־״̬	
float gpsdata[2];
}LongLatidata;
LongLatidata latdata;
float tempdata[2];
char latStrAF[64];          //������ݾ�γ����������
char lonStrAF[64];   //������ݾ�γ��������ʾ
//////////////////////////���Ǿ�ƫ����///////////////////////////////////


void Clear_Buffer(void)//��մ���2����
{
    printf(buf_uart2.buf);  //���ǰ��ӡ��Ϣ
    Delay(300);
    buf_uart2.index=0;
    memset(buf_uart2.buf,0,BUFLEN);
}

int BC20_Init(void)
{
    int errcount = 0;
    err = 0;    //�ж�ģ�鿨�Ƿ��������Ҫ
    printf("start init BC20\r\n");
    Uart2_SendStr("AT\r\n");
    Delay(300);
    printf(buf_uart2.buf);      //��ӡ�յ��Ĵ�����Ϣ
    printf("get back BC20\r\n");
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//����OK
    Clear_Buffer();	
    while(strx==NULL)
    {
        printf("\r\n��Ƭ���������ӵ�ģ��...\r\n");
        Clear_Buffer();	
        Uart2_SendStr("AT\r\n");
        Delay(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//����OK
    }
    
    Uart2_SendStr("AT+CIMI\r\n");//��ȡ����
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"ERROR");//ֻҪ�������� �����ͳɹ�
    if(strx==NULL)
    {
        printf("�ҵĿ����� : %s \r\n",&buf_uart2.buf[8]);
        Clear_Buffer();	
        Delay(300);
    }
    else
    {
        err = 1;
        printf("������ : %s \r\n",buf_uart2.buf);
        Clear_Buffer();
        Delay(300);
    }

    Uart2_SendStr("AT+CGATT=1\r\n");//�������磬PDP�����ƶ��ն����ӵ��������
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//��OK
    Clear_Buffer();	
    if(strx)
    {
        Clear_Buffer();	
        printf("init PDP OK\r\n");
        Delay(300);
    }
    Uart2_SendStr("AT+CGATT?\r\n");//��ѯ����״̬
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//��1 ��������ɹ� ��ȡ��IP��ַ��
    Clear_Buffer();	
    errcount = 0;
    while(strx==NULL)
    {
        errcount++;
        Clear_Buffer();	
        Uart2_SendStr("AT+CGATT?\r\n");//��ȡ����״̬
        Delay(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//����1,����ע���ɹ�
        if(errcount>100)     //��ֹ��ѭ��
        {
            err=1;
            errcount = 0;
            break;
        }
    }


    Uart2_SendStr("AT+QBAND?\r\n"); //�������ֵ
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//����OK
    if(strx)
    {
        printf("========BAND========= \r\n %s \r\n",buf_uart2.buf);
        Clear_Buffer();
        Delay(300);
    }

    Uart2_SendStr("AT+CSQ\r\n");//�鿴��ȡCSQֵ���ж��ź�ǿ��
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+CSQ");//����CSQ
    if(strx)
    {
        printf("�ź�����:%s\r\n",buf_uart2.buf);
        Clear_Buffer();
        Delay(300);
    }
    Uart2_SendStr("AT+QCCID\r\n");
    Delay(300);
    Clear_Buffer();	

    return err;
}

void BC20_PDPACT(void)//�������Ϊ���ӷ�������׼��
{
    int errcount = 0;
    Uart2_SendStr("AT+CGPADDR=1\r\n");//������������豸IP
    Delay(300);
    Clear_Buffer();
    Uart2_SendStr("AT+CGSN=1\r\n");//�����������IMEI���кź������Ϣ
    Delay(300);
    Clear_Buffer();
    Uart2_SendStr("AT+CGATT?\r\n");//�������ָʾ����״̬
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//ע����������Ϣ
    Clear_Buffer();	
    while(strx==NULL)
    {
        errcount++;
        Clear_Buffer();
        Uart2_SendStr("AT+CGATT?\r\n");//�����
        Delay(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGATT: 1");//һ��Ҫ�ն�����
        if(errcount>100)     //��ֹ��ѭ��
        {
            errcount = 0;
            break;
        }
    }
    Delay(300);
    Clear_Buffer();	
    Delay(300);
}


void BC20_INITGNSS(void)//����GPS
{
    int errcount = 0;
    Uart2_SendStr("AT+QGNSSC=1\r\n");//����GPS Ҫ�ȴ��ܾ�����GNSS
    Delay(1000);Delay(1000);Delay(1000);
    Clear_Buffer();
    Uart2_SendStr("AT+QGNSSC?\r\n");//��ѯGPS�������
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QGNSSC: 1");//�����ɹ�
    Clear_Buffer();
    while(strx==NULL)
    {
        errcount++;
        Clear_Buffer();
        Uart2_SendStr("AT+QGNSSC?\r\n");//��ѯ
        Delay(1000);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QGNSSC: 1");//�����ɹ�
        if(errcount>100)     //��ֹ��ѭ��
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
		Uart2_SendStr("AT+QGNSSRD=\"NMEA/RMC\"\r\n");//��ѯ����״̬
		delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)"$GNRMC");//��1
		
		while(strx==NULL)
		{
				Clear_Buffer();	
				Uart2_SendStr("AT+QGNSSRD=\"NMEA/RMC\"\r\n");//��ȡ����״̬
				delay_ms(300);
				strx=strstr((const char*)buf_uart2.buf,(const char*)"$GNRMC");//����1,����ע���ɹ�
		}
		sprintf(GPRMCSTR,"%s",strx);
	

		OLED_ShowString(0,2,"START GPS [OK] ");
		Clear_Buffer();	//��ӡ�յ���GPS��Ϣ
		GPRMCSTR[2]=	'P';
		
		printf("============GETGPRMC==============\r\n%s",GPRMCSTR);		//��ӡGPRMC
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
�����ǽ������������
AT+QGNSSRD="NMEA/RMC"

+QGNSSRD: $GNRMC,091900.00,A,2603.9680,N,11912.4174,E,0.189,,201022,,,A,V*1A

OK
*****************************************************/


//��GPS������
// $GNRMC,091900.00,A,2603.9680,N,11912.4174,E,0.189,,201022,,,A,V*1A
int Get_GPSdata()
{
		int i=0;
    strx=strstr((const char*)GPRMCSTR,(const char*)"A,");//��ȡγ�ȵ�λ��
       if(strx)
        {
            for(i=0;i<9;i++)
            {
             latdata.Latitude[i]=strx[i+2];//��ȡγ��ֵ2603.9576
            }
						strx=strstr((const char*)GPRMCSTR,(const char*)"N,");//��ȡ����ֵ
						if(strx)
						{
								 for(i=0;i<10;i++)	//��ȡ���� 11912.4098
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
			Getdata_Change(latdata.getstautus);//���ݻ���
			Clear_Buffer();
		 return 0;

}

/*************��������γ������,Ȼ��ֱ���ύ����*******************/	

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
		     (float)(latdata.longitudedd[4]-0x30)/100+(float)(latdata.longitudedd[5]-0x30)/1000+(float)(latdata.longitudedd[6]-0x30)/10000)/60.0;//��ȡ����������
       
///////////////////////////////////////////
				for(i=0;i<2;i++)
						latdata.Latitudess[i]=latdata.Latitude[i];
				for(i=2;i<9;i++)
						latdata.Latitudedd[i-2]=latdata.Latitude[i];	
				 
			latdata.gpsdata[1]=(float)(latdata.Latitudess[0]-0x30)*10+(latdata.Latitudess[1]-0x30)\
		     +((latdata.Latitudedd[0]-0x30)*10+(latdata.Latitudedd[1]-0x30)+(float)(latdata.Latitudedd[3]-0x30)/10+\
		     (float)(latdata.Latitudedd[4]-0x30)/100+(float)(latdata.Latitudedd[5]-0x30)/1000+(float)(latdata.Latitudedd[6]-0x30)/10000)/60.0;//��ȡ����������b

	
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
    Uart2_SendStr(atstr);//����0 socketIP�Ͷ˿ں������Ӧ���ݳ����Լ�����
    Delay(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//����OK
    while(strx==NULL)
    {
        errcount++;
        strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//����OK
        if(errcount>100)     //��ֹ��ѭ��
        {
            errcount = 0;
            break;
        }
    }

    Clear_Buffer();	
}


void BC20_RegALIYUNIOT(void)//ƽ̨ע��
{
    u8  BC20_IMEI[20],i;//IMEIֵ
    int errcount = 0;
    Uart2_SendStr("AT+QMTDISC=0\r\n");//Disconnect a client from MQTT server
    delay_ms(300);
    Clear_Buffer();

    Uart2_SendStr("AT+QMTCLOSE=0\r\n");//ɾ�����
    delay_ms(300);
    Clear_Buffer();

    memset(atstr,0,BUFLEN);
    sprintf(atstr,"AT+QMTCFG=\"ALIAUTH\",0,\"%s\",\"%s\",\"%s\"\r\n",PRODUCEKEY,DEVICENAME,DEVICESECRET);
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);//���Ͱ��������ò���
    delay_ms(300);  //�ȴ�300ms����OK
    strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//��OK
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"OK");//��OK
    }
    Clear_Buffer();

    Uart2_SendStr("AT+QMTOPEN=0,\"iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883\r\n");//��¼������ƽ̨
    delay_ms(3000);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTOPEN: 0,0");//��+QMTOPEN: 0,0
    while(strx==NULL)
    {
        errcount++;
        delay_ms(300);
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTOPEN: 0,0");//����OK
        if(errcount>500)     //��ֹ��ѭ��
        {
            GPIO_SetBits(GPIOC,GPIO_Pin_6);		//ģ������
            Delay(500);
            GPIO_ResetBits(GPIOC,GPIO_Pin_6);
            Delay(300);
            NVIC_SystemReset();
        }
    }
    Clear_Buffer();

    Uart2_SendStr("AT+CGSN=1\r\n");//��ȡģ���IMEI��
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGSN:");//��+CGSN:
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+CGSN:");//��+CGSN:
    }
    for(i=0;i<15;i++)
        BC20_IMEI[i]=strx[i+7];
    BC20_IMEI[15]=0;
    Clear_Buffer();

    printf("�ҵ�ģ��IMEI��:%s\r\n",BC20_IMEI);

    memset(atstr,0,BUFLEN);
    sprintf(atstr,"AT+QMTCONN=0,\"%s\"\r\n",DEVICENAME);//����MQTT������
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);//�������ӵ�������
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTCONN: 0,0,0");//��+QMTCONN: 0,0,0
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTCONN: 0,0,0");//��+QMTCONN: 0,0,0
    }
    Clear_Buffer();

    memset(atstr,0,BUFLEN);
    sprintf(atstr,"AT+QMTSUB=0,1,\"/%s/%s/user/get\",0 \r\n",PRODUCEKEY,DEVICENAME);
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);//�������ӵ�������
    delay_ms(300);
    Clear_Buffer();
}


void BC20_ALYIOTSenddata2(u8 *len,u8 *data)//�Ϸ����ݣ��Ϸ������ݸ���Ӧ�Ĳ���й�ϵ���û���Ҫע����Ȼ���Ӧ���ݼ���
{
    memset(atstr,0,BUFLEN);
    sprintf(atstr,"AT+QMTPUB=0,1,1,0,\"/sys/%s/%s/thing/event/property/post\",\"{params:{LightLux:%s}}\"\r\n",PRODUCEKEY,DEVICENAME,data);
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);
    delay_ms(300);
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTPUB: 0,1,0");//��SEND OK
    while(strx==NULL)
    {
        strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTPUB: 0,1,0");//��SEND OK
    }
    Clear_Buffer();
}


char send_json[BUFLEN]; //�齨�����ݵ�JSON
u8 Mqttaliyun_Savedata(u8 *t_payload,int temp,int humi,char *latstr,char *lonstr)
{

	  //����json���ܳ���
    int json_len;
	  //���͵�json��ʽ
    char json[]="{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"CurrentTemperature\":{\"value\":%d},\"CurrentHumidity\":{\"value\":%d},\"GeoLocation\":{ \"value\":{\"Latitude\":%s,\"Longitude\":%s,\"Altitude\":1,\"CoordinateSystem\":1}}},\"method\":\"thing.event.property.post\"}";	 
		memset(send_json,0,BUFLEN);
    sprintf(send_json, json, temp, humi,latstr,lonstr); //�齨��Ҫ���͵�json ����
    json_len = strlen(send_json)/sizeof(char);
  	memcpy(t_payload, send_json, json_len);
    return json_len;
}

void CSTX_4G_ALYIOTSenddataGPS(int temp,int humi,char *latstr,char *lonstr)//�Ϸ����ݣ��Ϸ������ݸ���Ӧ�Ĳ���й�ϵ���û���Ҫע����Ȼ���Ӧ���ݼ���
{

		memset(send_json,0,BUFLEN);
    memset(atstr,0,BUFLEN);
	
		Clear_Buffer();	//��������֮ǰ���֮ǰ��ģ�鷴��������
    sprintf(atstr,"AT+QMTPUB=0,0,0,0,\"/sys/%s/%s/thing/event/property/post\"\r\n",PRODUCEKEY,DEVICENAME); //�ȷ�������
    printf("atstr = %s \r\n",atstr);
    Uart2_SendStr(atstr);//�������� ģ�鷢������
    delay_ms(300);
		strx=strstr((const char*)buf_uart2.buf,(const char*)">");//ģ�鷴�����Է���������
    while(strx==NULL)
    {
        errcount++;
        strx=strstr((const char*)buf_uart2.buf,(const char*)">");//ģ�鷴�����Է���������
				delay_ms(30);
        if(errcount>100)     //��ֹ��ѭ������
        {
            errcount = 0;
            break;
        }
    }
		
		Mqttaliyun_Savedata((u8*)send_json,temp,humi,latstr,lonstr);	//�齨CJSON����
		printf("send_json = %s \r\n",send_json);
		Uart2_SendStr((char *)send_json);	//�����������������ͽ����жϷ���
		delay_ms(30);
		UART2_send_byte(0x1A); //���ͽ�����
    strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTRECV: 0,0");//�����ͳɹ�
		errcount=0;
    while(strx==NULL)
    {
				errcount++;
				strx=strstr((const char*)buf_uart2.buf,(const char*)"+QMTRECV: 0,0");//�����ͳɹ�
				delay_ms(100);
				if(errcount>100)     //��ʱ�˳���ѭ�� ��ʾ����������ʧ��
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
        IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //ʹ�ܶԼĴ���IWDG_PR��IWDG_RLR��д����

        IWDG_SetPrescaler(prer);  //����IWDGԤ��Ƶֵ:����IWDGԤ��ƵֵΪ64

        IWDG_SetReload(rlr);  //����IWDG��װ��ֵ

        IWDG_ReloadCounter();  //����IWDG��װ�ؼĴ�����ֵ��װ��IWDG������

        IWDG_Enable();  //ʹ��IWDG
}
//ι�������Ź�
void IWDG_Feed(void)
{
        IWDG_ReloadCounter();//reload
}
