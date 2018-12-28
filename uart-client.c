#include "usbserial-file.h"
#include "serial.h"
#include <semaphore.h>
#include <time.h>

/********update log*********************
*2018-9-25 V1.0.2  add FileBlock_t timestamp member
				   add Resource temporarily unavailable error deel
************ end **********************/

#define VERSION "1.0.2"

void uartsend(char *buf,unsigned int len);
int uartrev(char *buf);

int MD5_file(const char *path,char *output)
{
    MD5_CTX ctx;
    char buf[4096];
    int rval;
    int fd = 0;
    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open file to read");
        return -1;
    }
    MD5_Init(&ctx);
    while(1)
    {
        rval = read(fd, buf, sizeof(buf));
        if (rval <= 0)
            break;
        MD5_Update(&ctx, buf, rval);
    }
    MD5_Final((unsigned char*)output, &ctx);
    close(fd);
    return 0;
}

unsigned long get_file_size(const char *path)  
{  
    unsigned long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}  

time_t get_file_mtime(const char *path)  
{  
    time_t file_mtime = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return file_mtime;  
    }else{  
        file_mtime = statbuff.st_mtime;  
    }  
    return file_mtime;  
}  

int sendfile(const char* file_path)  
{  
        int ret = 0;
        char sendbuf[MAXLINE] = {0};  
        char recvbuf[MAXLINE] = {0}; 
	int len            		  = 0;
	int i 					  = 0;
	int fd 					  = 0;
	int num = 0;
	Message_t* Message_repose = NULL;
	Message_t* Message        = (Message_t *)sendbuf;
	
	fd = open(file_path, O_RDONLY);
	if(fd < 0)
	{
		perror("file open failed"); 
		close(fd);
		return -1;
	}
	
	FileBlock_t *FileBlock = (FileBlock_t*)Message->buf;
	strcpy(FileBlock->file_name,file_path);
	MD5_file(file_path,FileBlock->md5);
	FileBlock->file_length = get_file_size(file_path);
	FileBlock->timestamp   = get_file_mtime(file_path);
	
	printf("file_length = %d\n",FileBlock->file_length);
	printf("timestamp = %ld\n",FileBlock->timestamp);
	/*for(i = 0;i < 16;i++)
		printf("%x",(uint8_t)FileBlock->md5[i]);*/
	
	Message->cmd     = CMD_SEND_FILEMSG_REQ;
	Message->type    = REQ_NOR;
	Message->buf_len = sizeof(FileBlock_t);
	uartsend((char*)Message,Message->buf_len+MESG_HEAD_LEN);
	
	bzero(sendbuf, MAXLINE);
	len = uartrev(sendbuf);
	if(len <= 0)
	{
		printf("uartrev fail\n");
	}
	
	Message_repose = (Message_t*)sendbuf;
	if(Message_repose->cmd    == CMD_SEND_FILEMSG_REQ
	   &&Message_repose->type == ACK_OK)
	{	
		bzero(sendbuf, MAXLINE);
		Message->cmd     = CMD_SEND_FILE;
		Message->type    = REQ_NORE;
/*		fd = open(file_path, O_RDONLY);
		if(fd < 0)
		{
			perror("open failed");  
			return -1;
		}
*/		
		len = read(fd, Message->buf,BUF_MAX_LEN);
		if (len <= 0)
				return -1;
		while(1)
		{
			Message->buf_len = len;
			printf("num = %d,buf_len = %d\n",num++,Message->buf_len);
			uartsend((char*)Message,Message->buf_len+MESG_HEAD_LEN);
			
			ret = uartrev(recvbuf);
			Message_repose = (Message_t*)recvbuf;
			if((ret != 0)&&(Message_repose->cmd  == CMD_SEND_FILE)
			&&(Message_repose->type  == ACK_OK))
			{
//			   printf("file send is ok\n");
				len = read(fd, Message->buf,BUF_MAX_LEN);
				if (len <= 0)
						break;
			}else if(Message_repose->type == ACK_NOSP)
			{
				 printf("server No space\r\n");
				 return -1;
			}else if(Message_repose->type == NO_INIT)
			{
				 printf("server NO_INIT\r\n");
				 return -1;
			}
			else
			{
			   printf("file send fail!retry send\n");
			   num--;
			   uartsend((char*)Message,Message->buf_len+MESG_HEAD_LEN);
			}
		}
		
		close(fd);
		printf("file send finish\n");
		
		bzero(sendbuf, MAXLINE);
	
		Message->cmd     = CMD_SEND_FINISH;
		Message->type    = REQ_NOR;
		Message->buf_len = 0;
		uartsend((char*)Message,MESG_HEAD_LEN);
		len = uartrev(sendbuf);
		Message_repose = (Message_t*)sendbuf;
		if(Message_repose->cmd  == CMD_SEND_FINISH
	    &&Message_repose->type  == ACK_OK)
	    {
		   printf("file send is ok\n");
	    }else
		{
		   printf("file send is fail\n");
		   printf("cmd = %d,type = %d\r\n",Message_repose->cmd,Message_repose->type);
		}
	}
	else
	{
		printf("Message_repose fail = %d\n",Message_repose->type);
	}
	
	return 0;
 /*     
	printf("send�?s\n",sendbuf); 
	uartsend(sendbuf,5);
	ret = uartrev(recvbuf);
	printf("len = %d rev:%s\n",ret,recvbuf);
	memset(sendbuf, 0, sizeof(sendbuf));  
	memset(recvbuf, 0, sizeof(recvbuf));  */
	
    
	
}  


/***********************udp send rev**********************************************/
#if 0
//char* SERVERIP = "192.168.24.138";  
char* SERVERIP = "10.0.0.2";  
#define ERR_EXIT(m) do{perror(m);}while(0)  
	
int sock; 
struct sockaddr_in servaddr,disservaddr,disp_local_addr; 

void udpsocketinit()
{
    struct timeval timeout = { 2, 0 };
    int ret;
	
     if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)  
     {
		perror("sock creat fail");
	}
    
	memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_port = htons(PORT);  
    servaddr.sin_addr.s_addr = inet_addr(SERVERIP);
    
	//设置接收超时
	ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    if(ret == -1)
        perror("sock setsockopt");
	
}

void uartsend(char *buf,unsigned int len)
{
	sendto(sock, buf, len, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));  
}

int uartrev(char *buf)
{
	int length;
	char * server_ip = NULL;
    	unsigned short int port = 0;
	socklen_t addrlen = sizeof(struct sockaddr);
	
	length = recvfrom(sock, buf,BUF_MAX_LEN, 0, (struct sockaddr*)&disservaddr,&addrlen); 
	if (length == -1)  
	{   
		ERR_EXIT("recvfrom");  //如果设置了超时，超过3S没有获取到数据就会报�?Resource temporarily unavailable
		return -1;
	}
	
//	server_ip = inet_ntoa(disservaddr.sin_addr);//display remove ip addr
//	port      = ntohs(disservaddr.sin_port);
//	printf("addrlen = %d server_ip = %s  port = %d::",addrlen,server_ip,port);
	  
	return length;
}
#endif

uint8_t Utils_CheckSum(const uint8_t *pdata, uint32_t length)
{
	uint8_t checksum = 0;

	while (length--)
	{
		checksum += *pdata++;
	}

	return checksum;
}

/***********************bsp send rev**********************************************/

typedef enum
{
	DATA_FRAME_STATE_IDLE = 0,
	DATA_FRAME_STATE_Prefix,
	DATA_FRAME_STATE_reserved,
	DATA_FRAME_STATE_len1,
    DATA_FRAME_STATE_Payload,
    DATA_FRAME_STATE_Checksum
}DATA_FRAME_STATE_e;
	

static int len = 0, frame_data_length = 0;;
static uint8_t frame_data[DATA_PACKET_PAYLOAD_LENGTH];
sem_t sem;

void rf_on_byte_received ( uint8_t byte )
{
    static uint8_t checksum = 0;
    uint32_t i;
    static DATA_FRAME_STATE_e rf_frame_state =	DATA_FRAME_STATE_IDLE;

    //memset(frame_data,0,MAX_SQUEUE_SIZE);
    //printf("%02x ",byte);

    switch ( rf_frame_state )
    {
    case DATA_FRAME_STATE_IDLE:
        frame_data_length = 0;
        checksum = 0;
        len = 0;

        if ( byte == 0x5a )
        {
            rf_frame_state =  DATA_FRAME_STATE_Prefix;
            checksum += byte;
        }
        else
        {
            rf_frame_state =  DATA_FRAME_STATE_IDLE;
            checksum = 0;
        }

        break;

    case DATA_FRAME_STATE_Prefix:
        rf_frame_state =  DATA_FRAME_STATE_reserved;//��ȡrssi�ź�
        checksum += byte;
        break;

    case DATA_FRAME_STATE_reserved:
        len = byte;
        rf_frame_state =  DATA_FRAME_STATE_len1;
        checksum += byte;
        break;

    case DATA_FRAME_STATE_len1:
        len |= ( byte << 8 );
        rf_frame_state =  DATA_FRAME_STATE_Payload;
        checksum += byte;
        break;
		
    case DATA_FRAME_STATE_Payload:

        frame_data[frame_data_length++] = byte;
        checksum += byte;

        if ( frame_data_length == len - 1 )
        {
            rf_frame_state =  DATA_FRAME_STATE_Checksum;
        }

        break;

    case DATA_FRAME_STATE_Checksum:
        if ( byte == checksum )
        {
 //       	printf("checksum is ok byte = %0x,checksum = %0x\n",byte,checksum);
        	sem_post(&sem);       
        }else
        {
        	printf("byte = %0x,checksum = %d\n",byte,checksum);
        }
        rf_frame_state =  DATA_FRAME_STATE_IDLE;
        break;

    default:
        rf_frame_state =  DATA_FRAME_STATE_IDLE;
        break;
    }
}

void uart_send(char *buf,unsigned int len);

uint8_t DataPacketbuffer[DATA_PACKET_PAYLOAD_LENGTH+56];
void DataPacket_send(uint8_t* pdata, uint16_t length)
{
	Uartfile_DataPacket_t *p_data_packet=(Uartfile_DataPacket_t*)DataPacketbuffer;

	p_data_packet->Prefix      = 0x5a5a;
    p_data_packet->Length      = length + FRAME_RESERVED_FOR_CHECKSUM_LENGTH;

    memcpy(p_data_packet->Payload, pdata, length);
	
    p_data_packet->Payload[p_data_packet->Length - 1] = Utils_CheckSum((uint8_t*)p_data_packet,
                                                         p_data_packet->Length + DATA_PACKET_HEAD_LENGTH - 1);
	uart_send((char*)p_data_packet, p_data_packet->Length+DATA_PACKET_HEAD_LENGTH);
}



/***********************uart send rev**********************************************/
Uart_handle camera_handle;

void api_camera_received(int nread,uint8_t* buf)
{
	int i = 0;	
	
	for(i = 0;i < nread;i++)
	{
		rf_on_byte_received(buf[i]);
//		printf("%0x ",buf[i]);
	}
//	printf("\r\n");
//	printf("nread = %d,buf = %s\n",nread,buf);
//	write(radiouart_handle.uart_fd,buf,nread);
//	write(camera_handle.uart_fd,buf,nread);
}

void udpsocketinit()
{
  	char dev_node[] = "/dev/ttyUSB0";
//	printf("uart_handle size:%d\r\n",sizeof(fcuart_handle));
	
	camera_handle.uart_fd     = 0;
	camera_handle.device_node = dev_node;
	camera_handle.init.nSpeed = 115200;
	camera_handle.init.nBits  = 8;
	camera_handle.init.nEvent = 'N';
	camera_handle.init.nStop  = 1;
	camera_handle.uart_revdhandle = api_camera_received;
	
	UART_init(&camera_handle);


	sem_init(&sem,0,0);
}


void uart_send(char *buf,unsigned int len)
{
      write(camera_handle.uart_fd,buf,len);
}



/***********************file datapacket send rev**********************************************/


void uartsend(char *buf,unsigned int len)
{
      DataPacket_send((uint8_t *)buf,len);
}

int uartrev(char *buf)
{
	struct timespec abs_timeout;
	int i = 0;
	
 	clock_gettime(CLOCK_REALTIME, &abs_timeout);
	abs_timeout.tv_sec += 3	;

	if(sem_timedwait(&sem,&abs_timeout) == 0)//if(sem_wait(&sem) == 0)
	{
//		printf("sem_timedwait is ok\n");
		memcpy(buf,frame_data,frame_data_length);
	}
	else
	{
		printf("errno %d :\t\t%s\n",errno,strerror(errno));
		return 0;
	}
	return frame_data_length;
}



int main(int argc,char **argv)  
{
    printf("VERSION:%s\r\n",VERSION);    
	if (argc != 2)  
    {  
        printf("Usage: %s file_name\n", argv[0]);  
        exit(1);  
    } 
    udpsocketinit();

	sendfile(argv[1]);
	sleep(1);
//    while(1)
//	{
//		sleep(10);
//	}	
	
 //   close(sock);  	
    return 0;  
}




 
