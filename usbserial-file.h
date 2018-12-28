#ifndef __NET_FILE_H__
#define __NET_FILE_H__

#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <pthread.h>
#include <openssl/md5.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/time.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>  
#include <utime.h>
#include <errno.h>

#define    BUF_MAX_LEN    1024
#define    MAXLINE        (BUF_MAX_LEN+MESG_HEAD_LEN)
#define    PORT 		  8880

enum Command
{
    CMD_SEND_FILEMSG_REQ = 0x01,
	CMD_SEND_FILE 		 = 0x02,
	CMD_SEND_FINISH 	 = 0x03,
};

enum Type
{
	REQ_NOR  = 0x00,
    ACK_OK   = 0x01,
    ACK_FAIL = 0x02,
	ACK_NOSP = 0x03,  /* No space left on device */
	REQ_NORE = 0x04,
};

#define FILE_NAME_MAX_LEN 128
typedef struct
{
	char file_name[FILE_NAME_MAX_LEN];
    char md5[16];
    unsigned int file_length;
	time_t timestamp;
/*
    unsigned long file_name_len;
    int picture_num;
    int picture_total;
    int block_size;*/
} FileBlock_t;

#define MESG_HEAD_LEN 8
typedef struct
{
	uint16_t cmd;	
	uint16_t type;     //0 request 1 respose
	uint32_t buf_len;
	char     buf[BUF_MAX_LEN];
}Message_t;


#define FRAME_RESERVED_FOR_CHECKSUM_LENGTH 1
#define DATA_PACKET_HEAD_LENGTH               4
#define DATA_PACKET_PAYLOAD_LENGTH     BUF_MAX_LEN

typedef struct
{
    uint16_t Prefix;                                     /* 同步字节 */
    uint16_t Length;                                     /* 数据负载长度，包含checksum */
    uint8_t  Payload[DATA_PACKET_PAYLOAD_LENGTH];
}Uartfile_DataPacket_t;  


typedef void (*CmdOps)(Message_t*);  

typedef struct 
{
	unsigned char cmd;
	unsigned char type;
	CmdOps    cmd_ops;
}cmd_repose_t;

#endif

