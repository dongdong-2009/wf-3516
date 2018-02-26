#ifndef	__LKT_H__
#define	__LKT_H__

void lkt4200_init(int serial_fd);
void lkt4200(int serial_fd);
char* cmd_system(const char* command);
int lkt4200_writeNvm(int serial_fd,unsigned int  addr,unsigned char *data);
int LK_ReadNvm(int serial_fd,unsigned int addr,unsigned char *ReadBut);
int deviceActivate(int serial_fd);
int deviceTestActivate(int serial_fd);
// unsigned char lkt4200ks(int serial_fd,unsigned char ks);
int checkLKT4200(int serial_fd);
#endif
