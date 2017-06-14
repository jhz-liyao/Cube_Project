#include "main.h"
#include "mfrc522.h"
#include "stdint.h" 
#include "Log.h"
#include "spi.h"
#include "TaskTimeManager.h"
//#include <string.h>
//#include <stdio.h>
#define MAXRLEN 18          
/*�����ӿ�*/ 
/******** MFRC522 PIN description ************
*	SPI.NSS==UART.RX==IIC.SDA
*	SPI.SCK==UART.DTRQ==IIC.ADR1
*	SPI.SI==UART.MX==IIC.ADR0
*	SPI.SO==UART.TX==IIC.SCL
* *******************************************/
//#define     SET_RC522_RST  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET)
//#define     CLR_RC522_RST  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET)
//#define     RC522_SO   HAL_GPIO_ReadPin(MISO_GPIO_Port, MISO_Pin)	
//#define     SET_RC522_SI  HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_SET)     
//#define     CLR_RC522_SI  HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_RESET)
//#define     SET_RC522_SCK  HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_SET)
//#define     CLR_RC522_SCK  HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_RESET)
//#define     SET_RC522_NSS  HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_SET)
//#define     CLR_RC522_NSS  HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_RESET)

#define RST522_1 HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET)
#define RST522_0 HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET)
#define NSS_1() HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_SET)
#define NSS_0() HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_RESET)
#define _NOP() __NOP()

//void test2(){CLR_RC522_SI;CLR_RC522_SCK;SET_RC522_NSS;while(1);}
/////////////////////////////////////////////////////////////////////
//��    �ܣ�Ѱ��
//����˵��: req_code[IN]:Ѱ����ʽ
//                0x52 = Ѱ��Ӧ�������з���14443A��׼�Ŀ�
//                0x26 = Ѱδ��������״̬�Ŀ�
//          pTagType[OUT]����Ƭ���ʹ���
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
   char status;  
   unsigned int  unLen;
   unsigned char ucComMF522Buf[MAXRLEN]; 
	
   ClearBitMask(Status2Reg,0x08);
   WriteRawRC(BitFramingReg,0x07);
   SetBitMask(TxControlReg,0x03);
 
   ucComMF522Buf[0] = req_code;

   status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);

   if ((status == MI_OK) && (unLen == 0x10))
   {    
       *pTagType     = ucComMF522Buf[0];
       *(pTagType+1) = ucComMF522Buf[1];
   }
   else
   {   status = MI_ERR;   }
   
   return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�����ײ
//����˵��: pSnr[OUT]:��Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////  
char PcdAnticoll(unsigned char *pSnr)
{
    char status;
    unsigned char i,snr_check=0;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    

    ClearBitMask(Status2Reg,0x08);
    WriteRawRC(BitFramingReg,0x00);
    ClearBitMask(CollReg,0x80);
 
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x20;

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);

    if (status == MI_OK)
    {
    	 for (i=0; i<4; i++)
         {   
             *(pSnr+i)  = ucComMF522Buf[i];
             snr_check ^= ucComMF522Buf[i];
         }
         if (snr_check != ucComMF522Buf[i])
         {   status = MI_ERR;    }
    }
    
    SetBitMask(CollReg,0x80);
    return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�ѡ����Ƭ
//����˵��: pSnr[IN]:��Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdSelect(unsigned char *pSnr)
{
    char status;
    unsigned char i;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    	ucComMF522Buf[6]  ^= *(pSnr+i);
    }
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
  
    ClearBitMask(Status2Reg,0x08);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
    
    if ((status == MI_OK) && (unLen == 0x18))
    {   status = MI_OK;  }
    else
    {   status = MI_ERR;    }

    return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���֤��Ƭ����
//����˵��: auth_mode[IN]: ������֤ģʽ
//                 0x60 = ��֤A��Կ
//                 0x61 = ��֤B��Կ 
//          addr[IN]�����ַ
//          pKey[IN]������
//          pSnr[IN]����Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////               
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr)
{
    char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+2] = *(pKey+i);   }
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
 //   memcpy(&ucComMF522Buf[2], pKey, 6); 
 //   memcpy(&ucComMF522Buf[8], pSnr, 4); 
    
    status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
    {   status = MI_ERR;   }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���ȡM1��һ������
//����˵��: addr[IN]�����ַ
//          pData[OUT]�����������ݣ�16�ֽ�
//��    ��: �ɹ�����MI_OK
///////////////////////////////////////////////////////////////////// 
char PcdRead(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
   
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
    if ((status == MI_OK) && (unLen == 0x90))
    {
        for (i=0; i<16; i++)
        {    *(pData+i) = ucComMF522Buf[i];   }
    }
    else
    {   status = MI_ERR;   }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�д���ݵ�M1��һ��
//����˵��: addr[IN]�����ַ
//          pData[IN]��д������ݣ�16�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////                  
char PcdWrite(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int  unLen;
    unsigned char i,ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
        
    if (status == MI_OK)
    {
        //memcpy(ucComMF522Buf, pData, 16);
        for (i=0; i<16; i++)
        {    ucComMF522Buf[i] = *(pData+i);   }
        CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   status = MI_ERR;   }
    }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ����Ƭ��������״̬
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
	char status=0;
	unsigned int  unLen;
	unsigned char ucComMF522Buf[MAXRLEN]; 

	ucComMF522Buf[0] = PICC_HALT;
	ucComMF522Buf[1] = 0;
	//CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

	return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ����Ƭ��������״̬
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char UnlockBlock0(void)
{
	char status=0;
	unsigned int  unLen;
	unsigned char ucComMF522Buf[4] = {0}; 
 
	ucComMF522Buf[0] = PICC_HALT;
	ucComMF522Buf[1] = 0;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
  if (status != MI_OK)
	{
		printf("PcdHalt != MI_OK\r\n");  
	}else{
		printf("������\r\n"); 
	}
	printf("recv1:%d  %X, %X, %X, %X\r\n", unLen,ucComMF522Buf[0],ucComMF522Buf[1],ucComMF522Buf[2],ucComMF522Buf[3]);
	unLen = 0;
	//-----------
//	ClearBitMask(Status2Reg,0x08);
//  SetBitMask(TxControlReg,0x03);
	
	
  WriteRawRC(BitFramingReg,0x07);
	memset(ucComMF522Buf, 0, 4);
  ucComMF522Buf[0] = 0x40;
	ucComMF522Buf[1] = 0x00;
  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);
	printf("recv1:%d  %X, %X, %X, %X\r\n", unLen,ucComMF522Buf[0],ucComMF522Buf[1],ucComMF522Buf[2],ucComMF522Buf[3]);
	unLen = 0;
	 
  
  WriteRawRC(BitFramingReg,0x00);
	memset(ucComMF522Buf, 0, 4);
	ucComMF522Buf[0] = 0x43;
  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);
	printf("recv1:%d  %X, %X, %X, %X\r\n", unLen,ucComMF522Buf[0],ucComMF522Buf[1],ucComMF522Buf[2],ucComMF522Buf[3]);
	unLen = 0;
	
	
	
	uint8_t data[] = {0xDE, 0x4B, 0xA2, 0x69, 0x5E, 0x08, 0x04, 0x00, 0x01, 0x0D, 0x63, 0xE3, 0xC1, 0xBB, 0x77, 0x1D};
	status = PcdWrite(0, data);
	printf("recv1:%d  %X, %X, %X, %X\r\n", unLen,ucComMF522Buf[0],ucComMF522Buf[1],ucComMF522Buf[2],ucComMF522Buf[3]);
	unLen = 0;
//	ucComMF522Buf[0] = 0xa0;
//	ucComMF522Buf[1] = 0x00;
//	ucComMF522Buf[2] = 0x5f;
//	ucComMF522Buf[3] = 0xb1; 
//	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);
//  if (status == MI_OK)
//  {    
//  } 

	return status;
}

/////////////////////////////////////////////////////////////////////
//��MF522����CRC16����
/////////////////////////////////////////////////////////////////////
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData)
{
    unsigned char i,n;
    ClearBitMask(DivIrqReg,0x04);
    WriteRawRC(CommandReg,PCD_IDLE);
    SetBitMask(FIFOLevelReg,0x80);
    for (i=0; i<len; i++)
    {   WriteRawRC(FIFODataReg, *(pIndata+i));   }
    WriteRawRC(CommandReg, PCD_CALCCRC);
    i = 0xFF;
    do 
    {
        n = ReadRawRC(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));
    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���λRC522
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdReset(void)
{
//    SET_RC522_RST;
//    DelayUS(10);
//    CLR_RC522_RST;
//    DelayUS(10);
//    SET_RC522_RST;
//    DelayUS(10);
    WriteRawRC(CommandReg,PCD_RESETPHASE);
    DelayUS(10);
    
    WriteRawRC(ModeReg,0x3D);            //��Mifare��ͨѶ��CRC��ʼֵ0x6363
    WriteRawRC(TReloadRegL,30);           
    WriteRawRC(TReloadRegH,0);
    WriteRawRC(TModeReg,0x8D);
    WriteRawRC(TPrescalerReg,0x3E);
    WriteRawRC(TxAutoReg,0x40);     
    return MI_OK;
}
//////////////////////////////////////////////////////////////////////
//����RC632�Ĺ�����ʽ 
//////////////////////////////////////////////////////////////////////
char M500PcdConfigISOType(unsigned char type)
{
	if (type == 'A')                     //ISO14443_A
	{ 
		ClearBitMask(Status2Reg,0x08);

 /*     WriteRawRC(CommandReg,0x20);    //as default   
       WriteRawRC(ComIEnReg,0x80);     //as default
       WriteRawRC(DivlEnReg,0x0);      //as default
	   WriteRawRC(ComIrqReg,0x04);     //as default
	   WriteRawRC(DivIrqReg,0x0);      //as default
	   WriteRawRC(Status2Reg,0x0);//80    //trun off temperature sensor
	   WriteRawRC(WaterLevelReg,0x08); //as default
       WriteRawRC(ControlReg,0x20);    //as default
	   WriteRawRC(CollReg,0x80);    //as default
*/
		WriteRawRC(ModeReg,0x3D);//3F
/*	   WriteRawRC(TxModeReg,0x0);      //as default???
	   WriteRawRC(RxModeReg,0x0);      //as default???
	   WriteRawRC(TxControlReg,0x80);  //as default???

	   WriteRawRC(TxSelReg,0x10);      //as default???
   */
		WriteRawRC(RxSelReg,0x86);//84
 //      WriteRawRC(RxThresholdReg,0x84);//as default
 //      WriteRawRC(DemodReg,0x4D);      //as default

 //      WriteRawRC(ModWidthReg,0x13);//26
		WriteRawRC(RFCfgReg,0x7F);   //4F
	/*   WriteRawRC(GsNReg,0x88);        //as default???
	   WriteRawRC(CWGsCfgReg,0x20);    //as default???
       WriteRawRC(ModGsCfgReg,0x20);   //as default???
*/
		WriteRawRC(TReloadRegL,30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
		WriteRawRC(TReloadRegH,0);
		WriteRawRC(TModeReg,0x8D);
		WriteRawRC(TPrescalerReg,0x3E);
		
  //     PcdSetTmo(106);
		DelayMS(10);
		PcdAntennaOn();
	}
	else{ return (char)-1; }

	return MI_OK;
}
/////////////////////////////////////////////////////////////////////
//��    �ܣ���RC632�Ĵ���
//����˵����Address[IN]:�Ĵ�����ַ
//��    �أ�������ֵ
/////////////////////////////////////////////////////////////////////
unsigned char ReadRawRC(unsigned char Address)
{
	unsigned char res = 0; 
	Address = ((Address<<1)&0x7E)|0x80;
//	NSS_0(); 
//	if(HAL_SPI_TransmitReceive(&hspi1, &Address, &res, 1, 100) != HAL_OK)
//		Log.error("HAL_SPI_TransmitReceive error\r\n");  
//		
//	NSS_1(); 
	NSS_0(); 
	if(HAL_SPI_Transmit(&hspi1, &Address, 1, 100) != HAL_OK)
		Log.error("HAL_SPI_Transmit error\r\n");  
	if(HAL_SPI_Receive(&hspi1, &res,  1, 100) != HAL_OK)
		Log.error("HAL_SPI_Receive error\r\n");  
	NSS_1(); 
	return res; 
     /*unsigned char i, ucAddr;
     unsigned char ucResult=0;

     CLR_RC522_SCK;
     CLR_RC522_NSS;
     ucAddr = ((Address<<1)&0x7E)|0x80;

     for(i=8;i>0;i--)
     {
				if((ucAddr&0x80)==0x80)
					SET_RC522_SI;
				else
					CLR_RC522_SI;
         SET_RC522_SCK;
         ucAddr <<= 1;
         CLR_RC522_SCK;
     }

     for(i=8;i>0;i--)
     {
         SET_RC522_SCK;
         ucResult <<= 1;
         ucResult |= RC522_SO;
         CLR_RC522_SCK;
     }

     SET_RC522_NSS;
     SET_RC522_SCK;
     return ucResult;*/
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�дRC632�Ĵ���
//����˵����Address[IN]:�Ĵ�����ַ
//          value[IN]:д���ֵ
/////////////////////////////////////////////////////////////////////
void WriteRawRC(unsigned char Address, unsigned char value)	//����SPI��д�ٶȺ���Ҫ����Ȼ�п���Ѱ������
{  
	Address = ((Address<<1)&0x7E);
	unsigned char data[] = {0,0};
	data[0] = Address;
	data[1] = value;
//	NSS_0();
//	if(HAL_SPI_Transmit(&hspi1, data, 2, 100) != HAL_OK)
//		Log.error("HAL_SPI_Transmit error1\r\n");   
//	NSS_1();
	NSS_0();
	if(HAL_SPI_Transmit(&hspi1, &Address, 1, 100) != HAL_OK)
		Log.error("HAL_SPI_Transmit error1\r\n"); 
	if(HAL_SPI_Transmit(&hspi1, &value, 1, 100) != HAL_OK)
		Log.error("HAL_SPI_Transmit error2\r\n");
	NSS_1(); 
	/*unsigned char i, ucAddr;
	CLR_RC522_SCK;
	CLR_RC522_NSS;
	ucAddr = ((Address<<1)&0x7E);

	for(i=8;i>0;i--)
	{
			if((ucAddr&0x80)==0x80)
				SET_RC522_SI;
			else
				CLR_RC522_SI;
			DelayUS(1);
			SET_RC522_SCK;
			ucAddr <<= 1;
			DelayUS(1);
			CLR_RC522_SCK;
			DelayUS(1);
	}

	for(i=8;i>0;i--)
	{
		if((value&0x80)==0x80)
			SET_RC522_SI;
		else
			CLR_RC522_SI;
			SET_RC522_SCK;
			DelayUS(1);
			value <<= 1;
			CLR_RC522_SCK;
			DelayUS(1);
	}
	SET_RC522_NSS;
	SET_RC522_SCK;*/
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���RC522�Ĵ���λ
//����˵����reg[IN]:�Ĵ�����ַ
//          mask[IN]:��λֵ
/////////////////////////////////////////////////////////////////////
void SetBitMask(unsigned char reg,unsigned char mask)  
{
    char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg,tmp | mask);  // set bit mask
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���RC522�Ĵ���λ
//����˵����reg[IN]:�Ĵ�����ַ
//          mask[IN]:��λֵ
/////////////////////////////////////////////////////////////////////
void ClearBitMask(unsigned char reg,unsigned char mask)  
{
    char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp & ~mask);  // clear bit mask
} 

/////////////////////////////////////////////////////////////////////
//��    �ܣ�ͨ��RC522��ISO14443��ͨѶ
//����˵����Command[IN]:RC522������
//          pInData[IN]:ͨ��RC522���͵���Ƭ������
//          InLenByte[IN]:�������ݵ��ֽڳ���
//          pOutData[OUT]:���յ��Ŀ�Ƭ��������
//          *pOutLenBit[OUT]:�������ݵ�λ����
/////////////////////////////////////////////////////////////////////
char PcdComMF522(unsigned char  Command, 
                  unsigned char *pInData, 
                  unsigned char InLenByte,
                  unsigned char *pOutData, 
                  unsigned int *pOutLenBit)
{
	char  status = MI_ERR;
	unsigned char  irqEn   = 0x00;
	unsigned char  waitFor = 0x00;
	unsigned char  lastBits;
	unsigned char  n;
	unsigned int i;
	switch (Command)
	{
			case PCD_AUTHENT:
				 irqEn   = 0x12;
				 waitFor = 0x10;
				 break;
			case PCD_TRANSCEIVE:
				 irqEn   = 0x77;
				 waitFor = 0x30;
				 break;
			default:
				 break;
	}
    
	WriteRawRC(ComIEnReg,irqEn|0x80);  //ʹ�ܽ��ܺͷ����ж�����
	ClearBitMask(ComIrqReg,0x80);     //��ComIrqRegΪ0xff,                
	WriteRawRC(CommandReg,PCD_IDLE); //ȡ����ǰ����
	SetBitMask(FIFOLevelReg,0x80);        
     
  for (i=0; i<InLenByte; i++)
  { 
		WriteRawRC(FIFODataReg, pInData[i]); }
    WriteRawRC(CommandReg, Command);
    if (Command == PCD_TRANSCEIVE)
    { 
			SetBitMask(BitFramingReg,0x80); 
		}          //��ʼ����
     i = 600;//����ʱ��Ƶ�ʵ���������M1�����ȴ�ʱ��25ms
     do 
     {
				n = ReadRawRC(ComIrqReg);
				i--;
				//printf("i=%d,n=0x%x,waitFor=%d\r\n",i,n,waitFor);
     }
     while ((i!=0) && !(n&0x01) && !(n&waitFor));
     ClearBitMask(BitFramingReg,0x80);  //���ͽ���

		if (i!=0)
		{  
			if(!(ReadRawRC(ErrorReg)&0x1B))
			{
				status = MI_OK;
				if (n & irqEn & 0x01)
				{ status = MI_NOTAGERR;  }//printf("n=%x\r\n",n);
				if (Command == PCD_TRANSCEIVE)
				{
					n = ReadRawRC(FIFOLevelReg);
					lastBits = ReadRawRC(ControlReg) & 0x07;   //�ó������ֽ��е���Чλ�����Ϊ0��ȫ��λ����Ч
					if (lastBits)
					{ *pOutLenBit = (n-1)*8 + lastBits; }
					else
					{ *pOutLenBit = n*8; }
					if (n == 0)
					{ n = 1; }
					if(n > MAXRLEN)
					{ n = MAXRLEN; }
					for (i=0; i<n; i++)
					{ pOutData[i] = ReadRawRC(FIFODataReg); }
				}
			}
			else
			{ status = MI_ERR; }         
		}
		//printf("return:status=%d,pOutLenBit=%d\r\n",status,*pOutLenBit);
		SetBitMask(ControlReg,0x80);           // stop timer now
		WriteRawRC(CommandReg,PCD_IDLE); 
		return status;
} 
char PcdComMF5222(unsigned char  Command, 
                  unsigned char *pInData, 
                  unsigned char InLenByte,
                  unsigned char *pOutData, 
                  unsigned int *pOutLenBit)
{
	char  status = MI_ERR;
	unsigned char  irqEn   = 0x00;
	unsigned char  waitFor = 0x00;
	unsigned char  lastBits;
	unsigned char  n;
	unsigned int i;
	switch (Command)
	{
			case PCD_AUTHENT:
				 irqEn   = 0x12;
				 waitFor = 0x10;
				 break;
			case PCD_TRANSCEIVE:
				 irqEn   = 0x77;
				 waitFor = 0x30;
				 break;
			default:
				 break;
	}
    
	WriteRawRC(ComIEnReg,irqEn|0x80);  //ʹ�ܽ��ܺͷ����ж�����
	ClearBitMask(ComIrqReg,0x80);     //��ComIrqRegΪ0xff,                
	WriteRawRC(CommandReg,PCD_IDLE); //ȡ����ǰ����
	SetBitMask(FIFOLevelReg,0x80);        
     
  for (i=0; i<InLenByte; i++)
  { 
		WriteRawRC(FIFODataReg, pInData[i]); }
    WriteRawRC(CommandReg, Command);
    if (Command == PCD_TRANSCEIVE)
    { 
			SetBitMask(BitFramingReg,0x80); 
		}          //��ʼ����
     i = 600;//����ʱ��Ƶ�ʵ���������M1�����ȴ�ʱ��25ms
     do 
     {
				n = ReadRawRC(ComIrqReg);
				i--;
				printf("i=%d,n=0x%x,waitFor=%d\r\n",i,n,waitFor);
     }
     while ((i!=0) && !(n&0x01) && !(n&waitFor));
     ClearBitMask(BitFramingReg,0x80);  //���ͽ���

		if (i!=0)
		{  
			if(!(ReadRawRC(ErrorReg)&0x1B))
			{
				status = MI_OK;
				if (n & irqEn & 0x01)
				{ status = MI_NOTAGERR;  }//printf("n=%x\r\n",n);
				if (Command == PCD_TRANSCEIVE)
				{
					n = ReadRawRC(FIFOLevelReg);
					lastBits = ReadRawRC(ControlReg) & 0x07;   //�ó������ֽ��е���Чλ�����Ϊ0��ȫ��λ����Ч
					if (lastBits)
					{ *pOutLenBit = (n-1)*8 + lastBits; }
					else
					{ *pOutLenBit = n*8; }
					if (n == 0)
					{ n = 1; }
					if(n > MAXRLEN)
					{ n = MAXRLEN; }
					for (i=0; i<n; i++)
					{ pOutData[i] = ReadRawRC(FIFODataReg); }
				}
			}
			else
			{ status = MI_ERR; }         
		}
		//printf("return:status=%d,pOutLenBit=%d\r\n",status,*pOutLenBit);
		SetBitMask(ControlReg,0x80);           // stop timer now
		WriteRawRC(CommandReg,PCD_IDLE); 
		return status;
} 
/////////////////////////////////////////////////////////////////////
//��������  
//ÿ��������ر����շ���֮��Ӧ������1ms�ļ��
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn()
{
    unsigned char i;
    i = ReadRawRC(TxControlReg);
    if (!(i & 0x03))
    {
        SetBitMask(TxControlReg, 0x03);
    }
		DelayMS(3);
}

/////////////////////////////////////////////////////////////////////
//�ر�����
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff()
{
  ClearBitMask(TxControlReg, 0x03);
	DelayMS(3);
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ��ۿ�ͳ�ֵ
//����˵��: dd_mode[IN]��������
//               0xC0 = �ۿ�
//               0xC1 = ��ֵ
//          addr[IN]��Ǯ����ַ
//          pValue[IN]��4�ֽ���(��)ֵ����λ��ǰ
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////                 
char PcdValue(unsigned char dd_mode,unsigned char addr,unsigned char *pValue)
{
    char status;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = dd_mode;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
        
    if (status == MI_OK)
    {
       // memcpy(ucComMF522Buf, pValue, 4);
			ucComMF522Buf[0]=pValue[0],ucComMF522Buf[1]=pValue[1],ucComMF522Buf[2]=pValue[2],ucComMF522Buf[3]=pValue[3];
 //       for (i=0; i<16; i++)
 //       {    ucComMF522Buf[i] = *(pValue+i);   }
        CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);
        unLen = 0;
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
        if (status != MI_ERR)
        {    status = MI_OK;    }
    }
    
    if (status == MI_OK)
    {
        ucComMF522Buf[0] = PICC_TRANSFER;
        ucComMF522Buf[1] = addr;
        CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]); 
   
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   status = MI_ERR;   }
    }
    return status;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�����Ǯ��
//����˵��: sourceaddr[IN]��Դ��ַ
//          goaladdr[IN]��Ŀ���ַ
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdBakValue(unsigned char sourceaddr, unsigned char goaladdr)
{
    char status;
    unsigned int  unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_RESTORE;
    ucComMF522Buf[1] = sourceaddr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
    
    if (status == MI_OK)
    {
        ucComMF522Buf[0] = 0;
        ucComMF522Buf[1] = 0;
        ucComMF522Buf[2] = 0;
        ucComMF522Buf[3] = 0;
        CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);
 
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
        if (status != MI_ERR)
        {    status = MI_OK;    }
    }
    
    if (status != MI_OK)
    {    return MI_ERR;   }
    
    ucComMF522Buf[0] = PICC_TRANSFER;
    ucComMF522Buf[1] = goaladdr;

    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }

    return status;
}