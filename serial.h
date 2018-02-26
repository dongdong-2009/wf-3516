#ifndef	__SERIAL_H__
#define	__SERIAL_H__

void set_baud(int fd, int baud);
int   lktSerialOpen      (const char *device, const int baud) ;
void  lktSerialClose     (const int fd) ;
void  serialFlush     (const int fd) ;
int   serialGetchar   (const int fd) ;
int   serialDataAvail (const int fd);
#endif