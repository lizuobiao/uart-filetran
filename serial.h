#ifndef __SERIAL_H_
#define __SERIAL_H_

#include <stdint.h>

//typedef void* Uart_revpthread(void *param);
//typedef void* Uart_revpthread(void *param);

typedef void Uart_revpthread(int nread,uint8_t* buf);

struct Uart_init{
	int nSpeed;		//波特率
	char nBits;		//数据位
	char nStop;		//停止位
	char nEvent;	//奇偶校验
	char rev;		//保留位
};
typedef struct UART_Handle{
	int   uart_fd;
	char *device_node;
	struct Uart_init init;
	Uart_revpthread * uart_revdhandle;
	pthread_t threadID;
}Uart_handle;

int UART_init(Uart_handle* uart_handle);


#endif
