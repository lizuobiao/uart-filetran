#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "serial.h"


int uart_init_port(const char * dev_node)
{
	int fd = 0;
	
	printf("dev_node = %s\r\n",dev_node);
	fd = open( dev_node, O_RDWR|O_NOCTTY|O_NDELAY);
	if (-1 == fd)
	{
		perror("Can't Open Serial Port");
		return -1;
	}
	else
	{
		printf("open ttyS1 is ok\n");
	}
 
    if(fcntl(fd, F_SETFL, 0)<0)
    {
    	perror("serial fcntl fail");
    }
    else
    {
        printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));
    }
	
    if(isatty(STDIN_FILENO)==0)
    {
    	perror("standard input is not a terminal device");
    }
    else
    {
//        printf("isatty success!\n");
    }
    printf("fd-open=%d\n",fd);
    return fd;
}


int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
   	termios newtio,oldtio;
    if  (tcgetattr( fd,&oldtio)  !=  0)
    {
    	perror("SetupSerial");
        return -1;
    }
    bzero(&newtio,sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch( nBits )
    {
	    case 7:
	        newtio.c_cflag |= CS7;
	        break;
	    case 8:
	        newtio.c_cflag |= CS8;
	        break;
    }

    switch( nEvent )
    {
	    case 'O':                     //濂囨牎楠?
	        newtio.c_cflag |= PARENB;
	        newtio.c_cflag |= PARODD;
	        newtio.c_iflag |= (INPCK | ISTRIP);
	        break;
	    case 'E':                     //鍋舵牎楠?
	        newtio.c_iflag |= (INPCK | ISTRIP);
	        newtio.c_cflag |= PARENB;
	        newtio.c_cflag &= ~PARODD;
	        break;
	    case 'N':                    //鏃犳牎楠?
	        newtio.c_cflag &= ~PARENB;
	        break;
	}

    switch( nSpeed )
    {
	    case 2400:
	        cfsetispeed(&newtio, B2400);
	        cfsetospeed(&newtio, B2400);
	        break;
	    case 4800:
	        cfsetispeed(&newtio, B4800);
	        cfsetospeed(&newtio, B4800);
	        break;
	    case 9600:
	        cfsetispeed(&newtio, B9600);
	        cfsetospeed(&newtio, B9600);
	        break;
	    case 115200:
	        cfsetispeed(&newtio, B115200);
	        cfsetospeed(&newtio, B115200);
	        break;
            case 1000000:
                cfsetispeed(&newtio, B1000000);
                cfsetospeed(&newtio, B1000000);
	        break;
	    default:
	        cfsetispeed(&newtio, B9600);
	        cfsetospeed(&newtio, B9600);
	        break;
    }
    if( nStop == 1 )
    {
        newtio.c_cflag &=  ~CSTOPB;
    }
    else if ( nStop == 2 )
    {
        newtio.c_cflag |=  CSTOPB;
    }
    newtio.c_cc[VTIME]  = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH);
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
    	perror("tcsetattr error");
        return -1;
    }
    return 0;
}

void * uart_revpthread(void *param)
{
	int rc,i,nread=0;
	Uart_handle *uart_handle = (Uart_handle*)param;
    int portfd = uart_handle->uart_fd;
	printf("portfd = %d\r\n",portfd);
    uint8_t buf[1024] = "1234\r\n";
	fd_set rset;
    int j;
    struct timeval tv;
	printf("revpthread-start\r\n");
	while(1)
	{
		FD_ZERO(&rset);
		FD_SET(portfd,&rset);
	
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		rc = select(portfd+1,&rset,NULL,NULL,&tv);
	
		if(rc == -1)
		{
		    continue;
		}
		if(rc == 0)
		{
		    continue;
		}
		else
		{

		    nread = read(portfd,buf,sizeof(buf));
		    if(nread == -1)
		    {
		    	perror("fc uart read fail");
		        usleep(100*1000);
		        break;
		    }
		    if(nread == 0)
		    {
		        printf("nread==0\n");
		        usleep(100*1000);
		        continue;
	   		}
//          printf("fc nread = %d\n",nread);
//			printf("buf = %0x",buf[0]);
			if(nread>0)
				uart_handle->uart_revdhandle(nread,buf);
		
		}
	}
}


int UART_init(Uart_handle* uart_handle)
{
	int Uart_handle_fd = 0,ret = 0;	
	if(	(uart_handle ==NULL) || (uart_handle->device_node == NULL)
		||uart_handle->uart_revdhandle == NULL)
		return -1;
	
	uart_handle->uart_fd = uart_init_port(uart_handle->device_node);
	if(uart_handle->uart_fd != -1)
	{
		ret = set_opt(uart_handle->uart_fd,uart_handle->init.nSpeed,uart_handle->init.nBits, 
				uart_handle->init.nEvent,uart_handle->init.nStop);
		if(ret == -1)
		{
			printf("set_opt fail");
			return -1;
		}
	}
	else
	{
		return -1;
	}

	pthread_create(&uart_handle->threadID, NULL, uart_revpthread,(void*)uart_handle);
	pthread_detach(uart_handle->threadID);//线程分离，进程终止时会自动回收资源。
	
	return ret;
}

Uart_handle fcuart_handle;
Uart_handle radiouart_handle;

void api_fcuart_received(int nread,uint8_t* buf)
{
//	printf("nread = %d,buf = %s\n",nread,buf);
//	write(fcuart_handle.uart_fd,buf,nread);
	write(radiouart_handle.uart_fd,buf,nread);
}

void ttys3_init()
{
	char dev_node[] = "/dev/ttyUSB1";
	
	fcuart_handle.uart_fd     = 0;
	fcuart_handle.device_node = dev_node;
	fcuart_handle.init.nSpeed = 1000000;
	fcuart_handle.init.nBits  = 8;
	fcuart_handle.init.nEvent = 'N';
	fcuart_handle.init.nStop  = 1;
	fcuart_handle.uart_revdhandle = api_fcuart_received;
	
	UART_init(&fcuart_handle);
}



void api_radiouart_received(int nread,uint8_t* buf)
{
//	printf("nread = %d,buf = %s\n",nread,buf);
//	write(radiouart_handle.uart_fd,buf,nread);
	write(fcuart_handle.uart_fd,buf,nread);
}

void ttys2_init()
{
	char dev_node[] = "/dev/ttyUSB1";
	
	radiouart_handle.uart_fd     = 0;
	radiouart_handle.device_node = dev_node;
	radiouart_handle.init.nSpeed = 115200;
	radiouart_handle.init.nBits  = 8;
	radiouart_handle.init.nEvent = 'N';
	radiouart_handle.init.nStop  = 1;
	radiouart_handle.uart_revdhandle = api_radiouart_received;
	
	UART_init(&radiouart_handle);
}
int test(void)
{
	ttys2_init();
//	ttys3_init();

	sleep(1000);
}






























