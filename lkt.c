#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "hiGpioReg.h"
#include "lkt.h"
#include "DES.h"
#include "serial.h"
#include <sys/time.h>

#ifdef YG_BOARD
//伊工的新版4200引脚
#define GPIO_RST    3
#define GPIO_NUM    0
#else
//WH-20170801
#define GPIO_RST    10
#define GPIO_NUM    0
#endif

//#define _PRINTF

char cmdRead[5] = {0x00,0xC0,0x00,0x00,0x09};

void lkt4200_init(int serial_fd)
{	

	gpioSetMode(GPIO_RST,GPIO_NUM,GPIO_OUTPUT,1);	  //lkt4200
    
	gpioSet(GPIO_RST,GPIO_NUM);
	usleep(20000);
	gpioClr(GPIO_RST,GPIO_NUM);
	usleep(20000);
	gpioSet(GPIO_RST,GPIO_NUM);
	usleep(200000);
	
	serialFlush(serial_fd);
}
int lktCmpString(unsigned char *str1,unsigned char *str2,int len)
{
	int i ;
	for(i=0 ;i < len ; i++)
	{
		if(str1[i] != str2[i])
			return -1;
	}
	return 0 ;
}
char* cmd_system(const char* command)
{
    char* result= NULL;
    FILE *fpRead;
    fpRead = popen(command, "r");
    char buf[1024];
    memset(buf,'\0',sizeof(buf));
	
    while(fgets(buf,1024-1,fpRead)!=NULL)
    { 
       result = buf;
    }
    if(fpRead!=NULL)
        pclose(fpRead);
    return result;
	
}
void lkt4200(int serial_fd)
{	
	int i ,counter=0;
    unsigned char Re_buf[20]; 
	static unsigned char inoutdata[128];
    static unsigned char keyStr[128];
	static unsigned char licenseStr[128];
	unsigned char cmd[5] = {0x80,0x08,0x00,0x00,0x0c};
	unsigned char writeNVM[20] = {0x05,0x00,0x00,0x10,0x4d,0x5C,0x01,0x84,0x2F ,0x37, 0xD9, 0x06, 0x51 ,0xF9,0x69, 0x37, 0x83, 0xC6 ,0x56,0x42};
	unsigned char writeBuf[12] = {0x07,0x00,0x00,0x08, 0x9C ,0x0D ,0x5F ,0xAD, 0xF5, 0xD2, 0x3F ,0x7D}; 

	
	memcpy(inoutdata,"\x08\x67\xcb\x05\xAF\xA9\x3B\x16\x15",0x09);
	memcpy(licenseStr,"\x08\x9c\x0d\x5f\xad\xf5\xd2\x3f\x7d",0x09);
	memcpy(keyStr,"\x4d\x5c\x01\x84\x2f\x37\xd9\x06\x51\xf9\x69\x37\x83\xc6\x56\x42",0x10);
	
	write(serial_fd,cmd,5);
	//等待返回
	while(serialGetchar(serial_fd) == 0x08);
	
	write(serial_fd,writeBuf,12);
	while(serialGetchar(serial_fd) == 0x61);
	serialFlush(serial_fd);

	write(serial_fd,cmdRead,5);
	memset(Re_buf,0,20);
	for(;;){
		
		Re_buf[counter] = serialGetchar(serial_fd); 
		
		if(Re_buf[0] == 0xC0)
		{
				
			if(Re_buf[counter] == 0x90)
			{
				serialFlush(serial_fd);
				break;
			}
			counter++;
		}
	}
		
	memset(inoutdata,0,128);
    for(i=0;i < 9 ; i++)
		inoutdata[i] = Re_buf[i+1];	
	
	decrypt_3des(inoutdata,keyStr);
	
	if(lktCmpString(inoutdata,licenseStr,9) < 0)
	{
		//printf("Oh My Goal!!! \n");
		system("rm -rf /app");
		system("rm -rf /usr/lib");
		system("reboot");
	}
	else
	{
	//	printf("4200 is ok\n");
	}
	
}

int lkt4200_writeNvm(int serial_fd,unsigned int  addr,unsigned char *data)
{
	unsigned char cmdStart[5] = {0x80,0x08,0x00,0x00,0x0C};
	char writeNVM[12] = {0x05,0x00,0x00,0x08};
	char Re_buf[20] ;
	
	unsigned char addrL,addrH ;
	
	// printf("addr is 0x%08X\n",addr);
	
	if(addr > 0x3900)
	{
		//printf("addr is over \naddr is 0x0000 -0x3900\n");
		return -1 ;
	}
	else{
		addrL = addr&0xFF;
		addrH = (addr&0xFF00)>>8;
	}

	//printf("addrH is 0x%02X addrL is 0x%02X\n",addrH,addrL);
	
	writeNVM[1] = addrH;
	writeNVM[2] = addrL;
	
	int i ;
	
	for(i = 0 ; i < 8 ; i++)
	{
		writeNVM[4+i] = 0x00;
	}
	
	int len = 8;//strlen(data); //注意该项...............................................
	for(i = 0 ; i < len ; i++)
	{
		writeNVM[4+i] = data[i];
	}
	
	
	write(serial_fd,cmdStart,5);
	//等待返回
	while(serialGetchar(serial_fd) == 0x80);
	serialFlush(serial_fd);
	while(serialGetchar(serial_fd) == 0x08);
#ifdef _PRINTF
	printf("cmd start is write done\n");
#endif
	write(serial_fd,writeNVM,12);
	//等待返回
	while(serialGetchar(serial_fd) == 0x05);
	serialFlush(serial_fd);
	while(serialGetchar(serial_fd) == 0x61);
#ifdef _PRINTF
	printf("cmd data is write done\n");
#endif
	return 0 ;
}

int LK_ReadNvm(int serial_fd,unsigned int addr,unsigned char *ReadBut)
{
	
	unsigned char cmdStart[5] = {0x80,0x08,0x00,0x00,0x03};
	unsigned char cmdData[3] = {0x06,0x00,0x00};
	unsigned char addrL,addrH ;
	unsigned char Re_buf[20] ;
	
	gpioSet(GPIO_RST,GPIO_NUM);
	usleep(20000);
	gpioClr(GPIO_RST,GPIO_NUM);
	usleep(20000);
	gpioSet(GPIO_RST,GPIO_NUM);
	usleep(200000);
	
	serialFlush(serial_fd);
	
	// printf("addr is 0x%08X\n",addr);
	if(addr > 0x4000)
	{
		//printf("addr is over \naddr is 0x0000 -0x3900\n");
		return -1 ;
	}
	else
	{
		addrL = addr&0xFF;
		addrH = (addr&0xFF00)>>8;
	}
	
	//printf("addrH is 0x%02X addrL is 0x%02X\n",addrH,addrL);
	cmdData[1] = addrH;
	cmdData[2] = addrL;
	
	write(serial_fd,cmdStart,5);

	//等待返回
	while(serialGetchar(serial_fd) == 0x80);
	serialFlush(serial_fd);
	while(serialGetchar(serial_fd) == 0x08);

	//printf("cmd start is write done\n");
	
	write(serial_fd,cmdData,3);

	//等待返回
	while(serialGetchar(serial_fd) == 0x80);
	serialFlush(serial_fd);

	char len = 0;
	
	for(;;){
		char c = serialGetchar(serial_fd); 

		if(c == 0x61)
		{  //  printf("done ....\n");
			len = serialGetchar(serial_fd); 
			//printf("len is 0x%02X\n",len);
			serialFlush(serial_fd);
			break;
		}
	}
	
	cmdRead[4] = len ;

	int n ;
	//printf("cmdRead:");
	//for(n=0 ; n < 5 ;n++)
	//	printf("0x%02X ",cmdRead[n]);
	//printf("\n");
	
	write(serial_fd,cmdRead,5);
	
	//等待返回
	
	while(serialGetchar(serial_fd) == 0xC0);
	while(serialGetchar(serial_fd) == len);

	int counter = 0;
	memset(Re_buf,0,20);	
	for(;;){
		
		Re_buf[counter] = serialGetchar(serial_fd); 
		if(Re_buf[0] == 0xC0)
		{
			// printf("counter is %d\n",counter);
			
			if(Re_buf[counter] == 0x90)
			{
				//printf("read done....\n");
				serialFlush(serial_fd);
				break;
			}
			counter++;
		}
		else if(Re_buf[0] == 0x6F){
			//printf("nvm no data \n");
			return 0 ;
		}
	}
	
	int i ;
	//printf("\n");
	for(i=0; i < len +3 ; i++)
	{
		//printf("0x%02X ",Re_buf[i]);
		ReadBut[i] = Re_buf[i];
	}
	//printf("\n");
	
	return  len +3 ;
}

int lkt4200Update(int serial_fd,unsigned int  addr,unsigned char *data)
{
	unsigned char cmdStart[5] = {0x80,0x08,0x00,0x00,0x0C};
	char writeNVM[12] = {0x05,0x00,0x00,0x08};
	//char Re_buf[20] ;
	unsigned char addrL,addrH ;
	
	gpioSet(GPIO_RST,GPIO_NUM);
	usleep(20000);
	gpioClr(GPIO_RST,GPIO_NUM);
	usleep(20000);
	gpioSet(GPIO_RST,GPIO_NUM);
	usleep(200000);
	
	serialFlush(serial_fd);
	
	addrL = addr&0xFF;
	addrH = (addr&0xFF00)>>8;
	
#ifdef _PRINTF
	printf("addrH is 0x%02X addrL is 0x%02X\n",addrH,addrL);
#endif
	writeNVM[1] = addrH;
	writeNVM[2] = addrL;
	
	int i ;

	for(i = 0 ; i < 8 ; i++)
	{
		writeNVM[4+i] = data[i];
	}
	
	// printf("data is :");
	// for(i = 0 ; i < 8 ; i++)
	// {
		// printf("0x%02X ",data[i]);
	// }
	// printf("\n");
	
	//for(i = 0 ; i < 12 ; i++){
	//	printf("0x%02X  " ,writeNVM[i]);
	//}
  //  printf("\n");
	
	write(serial_fd,cmdStart,5);
	//等待返回
	while(serialGetchar(serial_fd) == 0x80);
	serialFlush(serial_fd);
	while(serialGetchar(serial_fd) == 0x08);
#ifdef _PRINTF
	printf("cmd start is write done\n");
#endif
	write(serial_fd,writeNVM,12);
	//等待返回
	while(serialGetchar(serial_fd) == 0x05);
	serialFlush(serial_fd);
	while(serialGetchar(serial_fd) == 0x61);
#ifdef _PRINTF
	printf("cmd data is write done\n");
#endif

	return 0 ;
}
int checkLKT4200(int serial_fd){
	unsigned char serialDefault[8] = {0x8E,0xDF,0x10,0x11,0x0D,0x24,0x7A,0xD1};
	unsigned char Re_buf[20];
	memset(Re_buf, 0, 20);
	
	#ifdef _PRINTF
	printf("check LKT4200...\n");
	#endif

	//威锋底板
	if(LK_ReadNvm(serial_fd,0x3980,Re_buf) > 0 )
	{

		if(lktCmpString(Re_buf+2,serialDefault,8)<0)
		{
			#ifdef _PRINTF
				printf("you are kidding\n");
			#endif
			 system("reboot");
		}
		#ifdef _PRINTF
				printf("ok......\n");
		#endif
		
	}
	else 
	{
		//空白芯片处理
		#ifdef _PRINTF
			printf("LKT4200 is wrong \n");
		#endif
		 system("reboot");	
	}
	
	//绑定底板
	const char findEncrypt[]= "find /etc/ -name hostid";
	char* ptr = cmd_system(findEncrypt);

	#ifdef _PRINTF
			printf("find encrypt is %s\n",ptr);
	#endif
	
		//第一次启动调用
	if (ptr == NULL)
	{
#ifdef _PRINTF
		printf("first start!\n");
#endif			
		system("ppv");
		char *hostid = cmd_system("hostid | cut -f 7");
		#ifdef _PRINTF
			printf("hostid is %s\n",hostid);
		#endif

		lkt4200Update(serial_fd, 0x3910, hostid);
	}
	else {
		
		#ifdef _PRINTF
		printf("no first start!\n");
		#endif
		
		char *hostid = cmd_system("hostid | cut -f 7");
		
		memset(Re_buf, 0, 20);
		if (LK_ReadNvm(serial_fd, 0x3910, Re_buf) > 0)
		{
			if (lktCmpString(Re_buf + 2, hostid, 8) <  0)
			{
					#ifdef _PRINTF
					printf("no first start error!\n");
					#endif
				   system("reboot");
			}
			//else
			//printf("no first start okkkkkkk!\n");
		}
		else{
			   system("reboot");
		}
	}
	
	return 0 ;
	
}
int deviceTestActivate(int serial_fd)
{
	char *hostid = cmd_system("hostid | cut -f 7");
    unsigned char Re_buf[20] ={0};
	memset(Re_buf, 0, 20);

	if(LK_ReadNvm(serial_fd,0x3800,Re_buf) > 0 )
	{
		if(lktCmpString(Re_buf+2,hostid,8) < 0)
		{
#ifdef _PRINTF
			printf("error!!! \n");
#endif
			return -1 ;
		}
		else
		{
#ifdef _PRINTF
			printf("4200 is ok\n");
#endif
		}
	}
	else {
#ifdef _PRINTF
		printf("device not activate \n");	
#endif
		return -1 ;

	}
	return 0 ;
}


unsigned char lkt4200ks(int serial_fd,unsigned char ks)
{
	unsigned char data ;
	unsigned char cmdStart[5] = {0x80,0x08,0x00,0x00,0x05};
	unsigned char cmdData[5] = {0x0A,0x00,0x00,0x01};
	unsigned char addrL,addrH ;
	unsigned char Re_buf[20] ;
	unsigned int addr = 0x3980;
	static unsigned char inoutdata[128];
    static unsigned char keyStr[128];
	int i;
	
	lkt4200_init(serial_fd);
	
	addrL = addr&0xFF;
	addrH = (addr&0xFF00)>>8;

	cmdData[1] = addrH;
	cmdData[2] = addrL;
	cmdData[4] = ks ;

#ifdef _PRINTF
    int n ;
	printf("cmdData:");
	for(n=0 ; n < 5 ;n++)
		printf("0x%02X ",cmdData[n]);
	printf("\n");
#endif
	
	write(serial_fd,cmdStart,5);
	//等待返回
	
	while(serialGetchar(serial_fd) == 0x80);
	serialFlush(serial_fd);
	while(serialGetchar(serial_fd) == 0x08);
	
	write(serial_fd,cmdData,5);
	
	//等待返回
	while(serialGetchar(serial_fd) == 0x0A);
	serialFlush(serial_fd);
	char len = 0;
	for(;;){
		char c = serialGetchar(serial_fd); 
		if(c == 0x61)
		{   
			len = serialGetchar(serial_fd); 
#ifdef _PRINTF
			printf("len is 0x%02X\n",len);
#endif
			serialFlush(serial_fd);
			break;
		}
	}
	
	cmdRead[4] = len ;
	
#ifdef _PRINTF
	printf("cmdRead:");
	for(n=0 ; n < 5 ;n++)
		printf("0x%02X ",cmdRead[n]);
	printf("\n");
#endif

	write(serial_fd,cmdRead,5);
	//等待返回
	while(serialGetchar(serial_fd) != 0x09);
	
	int counter = 0;
	memset(Re_buf,0,20);	
	
	for(;;){
		
		Re_buf[counter] = serialGetchar(serial_fd); 
		if(Re_buf[0] == 0xC0)
		{			
			if(Re_buf[counter] == 0x90)
			{
				serialFlush(serial_fd);
				break;
			}
			counter++;
		}
	}
#ifdef _PRINTF
    printf("funtion 10 read: ");
	for(i=0; i < len +3 ; i++)
	{
		printf("0x%02X ",Re_buf[i]);
		
	}
	printf("\n");
#endif
	
    unsigned char cmd9[5] = {0x80,0x08,0x00,0x00,0x0C};
	unsigned char cmdData9[12] = {0x09,0x00,0x00,0x08 };
	cmdData9[1] = addrH;
	cmdData9[2] = addrL;

	
	for(i= 0 ; i < 8 ; i ++)
		cmdData9[4+i] = Re_buf[i+2];
	
#ifdef _PRINTF
	printf("cmdData9:");
	for(i=0 ;i < 12 ;i++)
		printf("0x%02X ",cmdData9[i]);
	printf("\n");
#endif
	
	write(serial_fd,cmd9,5);
	//等待返回
	while(serialGetchar(serial_fd) == 0x80);
	serialFlush(serial_fd);
	while(serialGetchar(serial_fd) == 0x08);
	
	write(serial_fd,cmdData9,12);
	while(serialGetchar(serial_fd) != 0x08);
	serialFlush(serial_fd);
	len = 0;
	for(;;){
		char c = serialGetchar(serial_fd); 
		if(c == 0x61)
		{   
			
			len = serialGetchar(serial_fd); 	
#ifdef _PRINTF
			printf("done ....\n");
			printf("len is 0x%02X\n",len);
#endif
			serialFlush(serial_fd);
			break;
		}
	}
	
	cmdRead[4] = len ;

	write(serial_fd,cmdRead,5);

	//等待返回
	while(serialGetchar(serial_fd) != 0x09);
	counter = 0;
	memset(Re_buf,0,20);	
	for(;;){
		
		Re_buf[counter] = serialGetchar(serial_fd); 
		if(Re_buf[0] == 0xC0)
		{
			if(Re_buf[counter] == 0x90)
			{
				serialFlush(serial_fd);
				break;
			}
			counter++;
		}
	}
#ifdef _PRINTF
	printf("LK_DESDecrypt:");
	for(i=0; i < len +3 ; i++)
	{
		printf("0x%02X ",Re_buf[i]);
		
	}
	printf("\n");
#endif
	
	return Re_buf[2];
}


int deviceActivate(int serial_fd)
{
	char *hostid = cmd_system("hostid | cut -f 7");
    int ret ;
	
	ret = lkt4200_writeNvm(serial_fd,0x3800,hostid);
#ifdef _PRINTF
	if(ret == 0){
		printf("device activate is ok\n");
	}
	else{
		printf("device activate is error\n");
	}
#endif
	return ret;
}




#ifdef _HAVECLOCK_




#define PERIOD_FLAG_ADDRESS 0x1010
#define PERIOD_INFO_ADDRESS 0x1020

#define INFO_SIZE 200
#define PERIOD_DAYS 7
#define SERIAL_BIT 8
#define ACTIVATE_BIT 6  
#define ACTIVATE_NUM 30
#define SERIAL_BIT_TOP (SERIAL_BIT+1)
#define ACTIVATE_BIT_TOP (ACTIVATE_BIT+1)

#define PERIOD_IDX_ADD  (ACTIVATE_NUM+1)  //激活类型
#define PERIOD_ACTIVATE_TIME_YMD_ADD  (ACTIVATE_NUM+2)  //注册日期--年-月-日
#define PERIOD_ACTIVATE_TIME_HMS_ADD  (ACTIVATE_NUM+3)  //注册日期--时-分-秒
enum ExpriodDays
{
	EX_DAY0, EX_DAY1, EX_DAY3, EX_DAY5, EX_DAY7, EX_DAY30, EX_DAY356, EX_DAY_FOREVER
};
int g_period_days[] = {0, 1, 3, 5, 7, 30, 356, 366};


int wrPeriodFlag(int serial_fd, char* period_buf, int wr_flag) //读试用标志  如果有读入激活码信息进4200  那么该标志位置WF
{

	
	if(0 == wr_flag)
	{
		int ret = lkt4200_writeNvm(serial_fd,PERIOD_FLAG_ADDRESS,period_buf);
		if(ret == 0)
			return 0;
		return -1;
	}
	else
	{
		if(LK_ReadNvm(serial_fd,PERIOD_FLAG_ADDRESS,period_buf) <= 0 )
		{
			return -1 ;
		}
	
		return 0 ;
	}
}

int processPeriodInfo(int serial_fd, char* path)
{
	
	char serialCode[SERIAL_BIT_TOP];	 //序列号
	char activateCode[ACTIVATE_NUM][ACTIVATE_BIT_TOP];	//激活码
	char activateNum;	//激活次数
	
	
	char MyStr[INFO_SIZE];
	
	
	int ret = wrPeriodFlag(serial_fd, "WF0000", 0);
	if(ret < 0 )
	{
		return -1;
	}
	
	memset(MyStr, 0, INFO_SIZE);
	//if(0 == wr_flag)
	{
		
		int fd1 = open(path, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
		ret = read(fd1, MyStr, INFO_SIZE);
		if(ret < 0 )
		{
			return -1;
		}
		close(fd1);
		//printf("read actInfo:%s\n", MyStr);
		
		memset(serialCode, 0, SERIAL_BIT_TOP);
		memcpy(serialCode, MyStr, SERIAL_BIT);
		//printf("serialCode:%s\n", serialCode);
		printf("write userInfo...\n");
		int i = 0;
		for ( i = 0; i < ACTIVATE_NUM;i++)
		{
			memset(activateCode[i], 0, ACTIVATE_BIT_TOP);
			memcpy(activateCode[i], &MyStr[SERIAL_BIT + i*ACTIVATE_BIT], ACTIVATE_BIT);
			printf("......\n");
			
			ret = lkt4200_writeNvm(serial_fd,PERIOD_INFO_ADDRESS+15*i,activateCode[i]);
			if(ret < 0)
				return -1;
			
			
		}
		char activateNumBuf[8];
		memset(activateNumBuf,0, 8);
		memcpy(activateNumBuf, &MyStr[SERIAL_BIT + ACTIVATE_NUM*ACTIVATE_BIT], 7);
		activateNum = atoi(activateNumBuf);
		//printf("activateNum:%s\n", activateNumBuf);
		ret = lkt4200_writeNvm(serial_fd,PERIOD_INFO_ADDRESS+15*ACTIVATE_NUM,activateNumBuf);
		if(ret < 0)
			return -1;
	}
	return 0;
	
}

int chechActivateCode(int* input_activate_code, char* activateCode, int *period_idx)
{
	int i = 0;
	int cur_activate_code[ACTIVATE_BIT];
	for (i = 0; i < ACTIVATE_BIT; i++)
	{
		char dat[2];
		memset(dat, 0, 2);
		memcpy(dat, &activateCode[i], 1);
		cur_activate_code[i] = atoi(dat);
	}
	
	int min_idx = 0;
	char min_dat = cur_activate_code[0];
	for (i = 1; i < ACTIVATE_BIT; i++)
	{
		if (min_dat > cur_activate_code[i])
		{
			min_dat = cur_activate_code[i];
			min_idx = i;
		}
	}
	
	
	int cnt = 0;

	int diff_idx = -1;
	for (i = 0; i < ACTIVATE_BIT; i++)
	{

		if (input_activate_code[i] == cur_activate_code[i])
		{
			cnt++;
		}
		else
		{
			diff_idx = i;
		}
	}
	if (cnt == 5)
	{
#ifdef _PRINTF
		printf("min_idx--diff_idx:%d,%d\n", min_idx, diff_idx);
#endif
		if(min_idx != diff_idx)
			cnt = 0;
		char dat2 = cur_activate_code[diff_idx];
		dat2 &= 0x07;
		//dat2 ^= input_activate_code[diff_idx];
		*period_idx = dat2^input_activate_code[diff_idx];
	}
	return cnt;
}



time_t convert(int year, int month, int day, int hour, int min, int sec)
{
	struct tm info = { 0 };
	info.tm_year = year - 1900;
	info.tm_mon = month - 1;
	info.tm_mday = day;
	info.tm_hour = hour;
	info.tm_min = min;
	info.tm_sec = sec;
	return mktime(&info);
}

int periodDay(int serial_fd, char* time, char* activate_time, int wr_flag) //年-月-日-时-分-秒
{
	char ymd_hms[15];
	memset(ymd_hms, 0, 15);
	if(wr_flag == 0)
	{
		//char time[] = "2017-09-01-23-02-45";
		int time_num = 0;
		int time_len = strlen(time);
		int i =0;
		for ( i = 0; i < time_len; i++)
		{
			if (time[i] == '-')
				continue;
			ymd_hms[time_num++] = time[i];
		}

		char writebuf[10];
		memset(writebuf,0, 10);
		memcpy(writebuf, ymd_hms, 8);
#ifdef _PRINTF
		printf("write YMDwritebuf:%s\n", writebuf);
#endif
		
		int ret = lkt4200_writeNvm(serial_fd,PERIOD_INFO_ADDRESS+15*(PERIOD_ACTIVATE_TIME_YMD_ADD),writebuf);
		if(ret < 0)
			return -1;
		
		memset(writebuf,0, 10);
		memcpy(writebuf, &ymd_hms[8], 6);
#ifdef _PRINTF
	printf("write HMSwritebuf:%s\n", writebuf);
#endif
		
		ret = lkt4200_writeNvm(serial_fd,PERIOD_INFO_ADDRESS+15*(PERIOD_ACTIVATE_TIME_HMS_ADD),writebuf);
		if(ret < 0)
			return -1;
		
		return 0;
	}
	else
	{
		
		unsigned char Re_buf[20] ={0};
		memset(Re_buf, 0, 20);
		if(LK_ReadNvm(serial_fd,PERIOD_INFO_ADDRESS+15*(PERIOD_ACTIVATE_TIME_YMD_ADD),Re_buf) > 0 )
		{
			//printf("YMDwritebuf:%s\n", Re_buf);
			memcpy(ymd_hms, Re_buf+2, 8);
			//printf("read YMDwritebuf:%s\n", ymd_hms);
		}
		else {
			return -1 ;
		}
		memset(Re_buf, 0, 20);
		if(LK_ReadNvm(serial_fd,PERIOD_INFO_ADDRESS+15*(PERIOD_ACTIVATE_TIME_HMS_ADD),Re_buf) > 0 )
		{
			//printf("HMSwritebuf:%s\n", Re_buf);
			memcpy(&ymd_hms[8], Re_buf+2, 6);
			//printf("read HMSwritebuf:%s\n", &ymd_hms[8]);
		}
		else {
			return -1 ;
		}
		
		int year, month, day, hour, min, sec;
		memset(Re_buf, 0, 20);memcpy(Re_buf, ymd_hms, 4);year = atoi(Re_buf);
		memset(Re_buf, 0, 20);memcpy(Re_buf, &ymd_hms[4], 2);month = atoi(Re_buf);
		memset(Re_buf, 0, 20);memcpy(Re_buf, &ymd_hms[6], 2);day = atoi(Re_buf);
		memset(Re_buf, 0, 20);memcpy(Re_buf, &ymd_hms[8], 2);hour = atoi(Re_buf);
		memset(Re_buf, 0, 20);memcpy(Re_buf, &ymd_hms[10], 2);min = atoi(Re_buf);
		memset(Re_buf, 0, 20);memcpy(Re_buf, &ymd_hms[12], 2);sec = atoi(Re_buf);
		
		sprintf(activate_time, "%04d-%02d-%02d-%02d-%02d-%02d", year, month, day, hour, min, sec);
		int fromSecond = (int)convert(year, month, day, hour, min, sec);

		sscanf(time, "%d-%d-%d-%d-%d-%d", &year, &month, &day, &hour, &min, &sec);
		int toSecond = (int)convert(year, month, day, hour, min, sec);
		int day_diff = (toSecond - fromSecond) / 24 / 3600;
		
		return day_diff;
	}
}

int diffDay(int serial_fd, char* cur_time, char* activate_time, int period_type)
{
	int year, month, day, hour, min, sec;
	sscanf(activate_time, "%d-%d-%d-%d-%d-%d", &year, &month, &day, &hour, &min, &sec);
	int fromSecond = (int)convert(year, month, day, hour, min, sec);
	sscanf(cur_time, "%d-%d-%d-%d-%d-%d", &year, &month, &day, &hour, &min, &sec);
	int toSecond = (int)convert(year, month, day, hour, min, sec);
	int diff_day = (toSecond - fromSecond) / 24 / 3600;
	int period_day = g_period_days[period_type];
	//printf("fromSecond:%d--toSecond:%d---period_day:%d\n", fromSecond, toSecond, period_day);
	if((diff_day >= period_day) || (toSecond <= fromSecond))
	{
		int ret = wrPeriodFlag(serial_fd, "WF0000", 0);
		if(ret < 0 )
			return 0;
		return 0;//试用结束
		
	}
	return (period_day - diff_day);
	
	
}

void parseActivateCode(char *in_activateCode, char *out_activateCode, unsigned char xor_dat)
{
	
	memset(out_activateCode, 0, ACTIVATE_BIT_TOP);
	int j = 0;
	for (j = 0; j < ACTIVATE_BIT / 2; j++)
	{
		char buf[3];
		memset(buf, 0, 3);
		memcpy(buf, &in_activateCode[j * 2], 2);
		//memcpy(buf, &bb[j * 2], 2);
		unsigned char hex = 0;
		sscanf(buf, "%02x", &hex);
		hex = hex ^ xor_dat;
		memset(buf, 0, 3);
		sprintf(buf, "%02x", hex);
		memcpy(&out_activateCode[j * 2], buf, 2);
	}

}
int verifyActivateCode(int serial_fd, int* input_activate_code, char* cur_time, int *period_type_int, int *activate_number)
{
	if(*activate_number >= ACTIVATE_NUM)
		return -1;
	char serialCode[SERIAL_BIT_TOP];	 //序列号
	char activateCode[ACTIVATE_NUM][ACTIVATE_BIT_TOP];	//激活码
	char activateNum = *activate_number;	//激活次数
	unsigned char Re_buf[20] ={0};
	memset(Re_buf, 0, 20);
	
	int period_idx = 0;
	memset(Re_buf, 0, 20);
	memset(activateCode[activateNum], 0, ACTIVATE_BIT_TOP);
	if(LK_ReadNvm(serial_fd,PERIOD_INFO_ADDRESS+activateNum*15,Re_buf) > 0 )
	{
		memcpy(activateCode[activateNum], Re_buf+2, ACTIVATE_BIT);
		char parse_activateCode[ACTIVATE_BIT_TOP];
		parseActivateCode(activateCode[activateNum], parse_activateCode, 0x55);
		//printf("input_activate_code:%d%d%d%d%d%d--activateCode[%d]:%s\n", input_activate_code[0],input_activate_code[1],input_activate_code[2],input_activate_code[3],input_activate_code[4],input_activate_code[5],activateNum, activateCode[activateNum]);
		int ret = chechActivateCode(input_activate_code, parse_activateCode, &period_idx);
		if(ret != 5)
		{
			//printf("failure:%d\n", period_idx);
			activateNum = ACTIVATE_NUM-1;
			memset(Re_buf, 0, 20);
			if(LK_ReadNvm(serial_fd,PERIOD_INFO_ADDRESS+(activateNum)*15,Re_buf) > 0 ) //判断输入码是否为永久激活
			{
				memset(activateCode[activateNum], 0, ACTIVATE_BIT_TOP);
				memcpy(activateCode[activateNum], Re_buf+2, ACTIVATE_BIT);
				char parse_activateCode[ACTIVATE_BIT_TOP];
				parseActivateCode(activateCode[activateNum], parse_activateCode, 0x55);
				ret = chechActivateCode(input_activate_code, parse_activateCode, &period_idx);
				if(ret == 6)
				{
					period_idx = EX_DAY_FOREVER;
				}
				else
					return -1;
			}
			else
				return -1;
		}
	}
	else {
		return -1 ;
	}
	activateNum++;
	*activate_number = activateNum;
	*period_type_int = period_idx;
	char writebuf[8];
	memset(writebuf,0, 8);
	sprintf(writebuf, "WF%02d%02d", period_idx, activateNum);
	//printf("write period_idx:%s\n", writebuf);
	int ret = wrPeriodFlag(serial_fd, writebuf, 0);
	if(ret < 0)
		return -1;
	ret = lkt4200_writeNvm(serial_fd,PERIOD_FLAG_ADDRESS,writebuf);
	if(ret < 0)
		return -1;
	ret = periodDay(serial_fd, cur_time, NULL, 0);
	if(ret < 0)
		return -1;
	return 0;
}
int getPeriodDayByPeriodType(int period_type_int)
{
	if(EX_DAY0 <= period_type_int && period_type_int <= EX_DAY_FOREVER)
		return g_period_days[period_type_int];
	return 0;
}
int getPeriodType(int serial_fd, int *period_type_int, int *activate_number)
{
	unsigned char Re_buf[20] ={0};
	memset(Re_buf, 0, 20);
	
	if(LK_ReadNvm(serial_fd,PERIOD_FLAG_ADDRESS,Re_buf) > 0 )
	{
		
		if(lktCmpString(Re_buf+2,"WF",2)==0)
		{
			char period_type[3];
			memset(period_type, 0, 3);
			memcpy(&period_type[0], Re_buf+2+2, 2);
			*period_type_int = atoi(period_type);
			//printf("period_type_int:%d\n", *period_type_int);
			memset(period_type, 0, 3);
			memcpy(&period_type[0], Re_buf+2+4, 2);
			*activate_number = atoi(period_type);
			//printf("activate_number:%d\n", *activate_number);
			
			return 0;
		}
		else
			return -1;	
	}
	else
		return -1 ;
}

int getPeriodDayOver(int serial_fd, int period_type, char *cur_time, char* activate_time)
{
	if(0 < period_type && period_type < EX_DAY_FOREVER)
	{
		int diff_day = periodDay(serial_fd, cur_time, activate_time, 1);
		int period_day = g_period_days[period_type];
		//printf("use_day:%d, period_day:%d\n", diff_day, period_day);
		if(diff_day >= period_day)
		{
			int ret = wrPeriodFlag(serial_fd, "WF0000", 0);
			if(ret < 0 )
				return 0;
			return 0;//试用结束
		
		}
		return (period_day - diff_day);
	}
	return -1;
}

int getMachineActivateType(int period_type)
{
	if(period_type == EX_DAY0) //还没有输入任何激活码
		return 0;
	if(period_type == EX_DAY_FOREVER) //已经永久激活
		return 1;
	else
		return 2;  //输入了其他试用的激活码
}

void readBCDHostId(char* hex_hostid)
{
	char *hostid = cmd_system("hostid | cut -f 7");
	if(hostid)
	{
		char Re_buf[3];
		int i=0;
		for(i=0; i<4; i++)
		{
			memset(Re_buf, 0, 3);
			memcpy(Re_buf, &hostid[i*2], 2);
			sscanf(Re_buf, "%02X", &hex_hostid[i]);
			//printf("hex_hostid:%02X\n", hex_hostid[i]);
		}
		
	}
}

int compareHostId(char* hex_hostid)
{
	char *hostid = cmd_system("hostid | cut -f 7");
	if(hostid)
	{
		char Re_buf[3];
		int i=0;
		for(i=0; i<4; i++)
		{
			memset(Re_buf, 0, 3);
			char hex=0;
			memcpy(Re_buf, &hostid[i*2], 2);
			sscanf(Re_buf, "%02X", &hex);
			if(hex_hostid[i] != hex)
				return -1;
			//printf("hex_hostid:%02X\n", hex_hostid[i]);
		}
		return 0;
		
	}
	
	return -1;
}
void clearMem(int serial_fd)
{
	wrPeriodFlag(serial_fd, "000000", 0);
	
}
#endif
