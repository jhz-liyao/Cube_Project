#ifndef __FRAME_CONFIG_H__
#define __FRAME_CONFIG_H__
#include "usart.h"
#define DEBUG_USART   huart1 //���Դ���
#define UART1_DMA_SENDER 1   //���ڷ�����
#define UART1_DMA_RECEIVER 1 //���ڽ�����
#define UART1_PROTOCOL_RESOLVER 1 //�������ݽ�����
#define LOG_OUT 1
#define PROTOCOL_VERSION  1  //1 E01�ϰ汾Э��  2 E01S�°汾Э��
#endif
