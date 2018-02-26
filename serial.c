#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/serial.h>
#include "hiGpioReg.h"
#include "serial.h"


void set_baud(int fd, int baud)
{
        int   status;
        struct termios   Opt;
        struct serial_struct Serial;
        tcgetattr(fd, &Opt);       
        tcflush(fd, TCIOFLUSH);
			
        cfsetispeed(&Opt, B115200);
        cfsetospeed(&Opt, B115200);
        tcflush(fd,TCIOFLUSH);       
        status = tcsetattr(fd, TCSANOW, &Opt); 
        if  (status != 0)
        {
                perror("tcsetattr fd1");
                return;
        }
        if((ioctl(fd,TIOCGSERIAL,&Serial))<0)
        {
                printf("Fail to get Serial!\n");
                return;
        }
        // printf("TIOCGSERIAL: OK.\n");
		
		Serial.flags = ASYNC_SPD_CUST;
        Serial.custom_divisor=Serial.baud_base/baud;
		
        // printf("divisor is %x\n",Serial.custom_divisor);
		
        if((ioctl(fd,TIOCSSERIAL,&Serial))<0)
        {
                printf("Fail to set Serial\n");
                return;
        }
        // printf("TIOCSSERIAL: OK.\n");
        ioctl(fd,TIOCGSERIAL,&Serial);
		// printf("\nBAUD: success set baud to %d,custom_divisor=%d,baud_base=%d\n",baud
		// ,Serial.custom_divisor,Serial.baud_base);
}

//调整波特率
#define BAUD_ADJUST    115451

int   lktSerialOpen(const char *device, const int baud) 
{
	struct termios opt; 
	struct serial_struct Serial;
	int     status, fd ;

	
    fd = open(device, O_RDWR|O_NOCTTY);    //默认为阻塞读方式

    if(fd == -1)
    {
        perror("open serial 0\n");
        system("reboot");
	    return -1 ;
    }
	
    tcgetattr(fd, &opt);      
    cfsetispeed(&opt, baud);
    cfsetospeed(&opt, baud);
	
    if(tcsetattr(fd, TCSANOW, &opt) != 0 )
    {     
       perror("tcsetattr error");
       return -1;
    }

    opt.c_cflag &= ~CSIZE;  
    opt.c_cflag |= CS8;     //8
  
	opt.c_cflag |= PARENB;   //E
	opt.c_cflag &= ~PARODD; 
	opt.c_iflag |= INPCK; 

	opt.c_cflag |= CSTOPB;  //2

    opt.c_cflag |= (CLOCAL | CREAD);
 
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 
    opt.c_oflag &= ~OPOST;
    opt.c_oflag &= ~(ONLCR | OCRNL);    
 
    opt.c_iflag &= ~(ICRNL | INLCR);
    opt.c_iflag &= ~(IXON | IXOFF | IXANY);    
    
    opt.c_cc[VTIME] = 2;
    opt.c_cc[VMIN] = 0;
    
    tcflush(fd, TCIOFLUSH);
    
    if(tcsetattr(fd, TCSANOW, &opt) != 0)
    {
        perror("serial error");
        return -1;
    }
	
	if(BAUD_ADJUST)
	{
		set_baud(fd,BAUD_ADJUST);
	}

	return fd ;
}

void  lktSerialClose(const int fd) 
{
	close (fd) ;
}

void serialFlush (const int fd)
{
  tcflush (fd, TCIOFLUSH) ;
}

int serialGetchar (const int fd)
{
  uint8_t x ;

  if (read (fd, &x, 1) != 1)
    return -1 ;

  return ((int)x) & 0xFF ;
}
int serialDataAvail (const int fd)
{
  int result ;

  if (ioctl (fd, FIONREAD, &result) == -1)
    return -1 ;

  return result ;
}