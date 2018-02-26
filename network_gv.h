
#ifndef _NETWORK_GV_H_
#define _NETWORK_GV_H_

typedef struct  {
	pthread_t			tid;		
	int					sock;
}CtlNetEnv;

int *g_sock;
#define DEVICE_ID					"IPCaabbccde"

int socket_open_listen( int port );
void* RemoteCfgThread( void *arg);
int start_network_control(pthread_t ctlThread);
void reset_netport();
int get_device_info( void * arg );
int set_network_lan( void* arg );
int get_network_lan( void* arg );
int set_reboot( void *arg );
void* thread_network_control();
#endif
