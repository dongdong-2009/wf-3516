/*
 * LANServer.c
 *
 *  Created on: 2011-6-22
 *      Author: zhangxian
 */

#include "LANServer.h"
#include "network_gv.h"

#define MULIT_CASE
#define MCASTADDR "233.0.0.1" //multicast address
#define MCASTPORT 5250 //multicast port

static int  m_iSocket = -1;
static pthread_t m_hThread = -1;

extern int g_upgrading;

static void * LANServer_thread(void * pVoid)
{
    struct sockaddr_in sockin;
    struct sockaddr_in sockin_server;
    unsigned int SOCK_SIZE = sizeof(sockin);
    int iRet = 0;
    struct NETDATA * pNetData = NULL;
    char pBuff[4096] = {0};
    const int SIZEOF_BUFF = sizeof(pBuff);
	AUTHORITY  au = AUTHORITY_NONE;
	int ver;
	in_addr_t local_ip;
	
    //
    while (true)
    {
		memset( pBuff, 0, SIZEOF_BUFF );
        iRet = recvfrom(m_iSocket, pBuff,SIZEOF_BUFF,  0, (struct sockaddr *)&sockin, &SOCK_SIZE);
		
        //
        if (iRet <= 0)
        {
            return NULL;
        }
		
		memcpy( &sockin_server, &sockin, sizeof(sockin_server) );

#ifdef MULIT_CASE
		sockin.sin_family = AF_INET;
		sockin.sin_port = htons(MCASTPORT);
		sockin.sin_addr.s_addr = inet_addr( MCASTADDR );
#endif
		
        pNetData = (struct NETDATA*)pBuff;
		
        if (LAN_SEARCH_DEVICE2 == pNetData->type)
		{			
			printf("LAN2 server get a client data! server: %s\n", inet_ntoa(sockin_server.sin_addr) );
			pNetData->type = LAN_SEARCH_DEVICE2_ACK;

			SYS_MSG   sm;
			memset( &sm,  0, sizeof(sm) );
			get_title( (void*)&sm );
			
			Network_Config_Data net;
			get_mac( &net );
			char s_mac[32] = {0};
			sprintf( s_mac, "%02X:%02X:%02X:%02X:%02X:%02X", net.MAC[0],net.MAC[1],net.MAC[2],net.MAC[3],net.MAC[4],net.MAC[5] );
			
			char s_version[64] = {0};
			if( g_upgrading )		
			{
				sprintf( s_version, "Upgrading..." );
			}
			else
			{
				SYS_MSG   sm2;
				memset( &sm2,  0, sizeof(sm2) );
				get_firmware_version( (void*)&sm2 );
				sprintf( s_version, sm2.data );
			}

			sprintf( pNetData->data, "ipc_tital:%s,mac:%s,firmware:%s,", sm.data, s_mac, s_version );
			int p2p = 0;
			if (p2p>=0)
			{
				sprintf( pNetData->data+strlen(pNetData->data), "p2p_online:%d,", p2p ); 
			}
			
			sprintf( pNetData->data+strlen(pNetData->data), "wifi:,", 0 ); 
						

			//sprintf( pNetData->data, "ipc_tital:%s,mac:%s,firmware:%s,stream_port:%d,", sm.data, s_mac, s_version, net.system_port );
			pNetData->len = sizeof(struct NETDATA)+strlen(pNetData->data);

			//printf( "LAN2: data len=%d, data=%s\n", pNetData->len, pNetData->data );
				

			DVR_TYPE_ID dev = get_device_type();
			if( dev < 0 )	dev = GV_IPCAM_TYPE;
			printf("dev type: %d \n", dev );
			
			pNetData->id = dev;			
			pNetData->operate = ( sockin_server.sin_addr.s_addr >> 24 ) & 0xFF;
			iRet = sendto(m_iSocket, (char*)pNetData, pNetData->len, 0, (struct sockaddr *)&sockin, SOCK_SIZE);
			//printf( "send to nvr, len:=%d \n",iRet );
		}
		else 
		{
			local_ip = net_get_ifaddr("eth0");
			//printf( "local ip: 0x%x\n", local_ip );
			
			if( -1 == local_ip )
				continue;
			
			unsigned short id = (local_ip & 0xffff0000) >> 16;
			
			//printf( "local ip address: 0x%x, local id %d, pNetData->id: %d\n", local_ip, id, pNetData->id );
			
			if( pNetData->id != id )
				continue;
			
			if( pNetData->type == LOGIN )
			{	
				ver = login( (char*)pNetData, &au );
				printf( "get au:%d\n", au );
				pNetData->type = LOGIN_ACK;
				
				printf( "should send back, mulitcast addr: 0x%x, port: %d\n",sockin.sin_addr.s_addr, sockin.sin_port );
				
				iRet = sendto(m_iSocket, (char*)pNetData, pNetData->len, 0, (struct sockaddr *)&sockin, SOCK_SIZE);
				
			}
			else if( pNetData->type == SET_NETWORK_LAN || pNetData->type == GET_NETWORK_LAN)
			{	
				printf( "au:%d, sizeof(NETWORK_LAN) = %d\n", au, sizeof(NETWORK_LAN) );
				
				//printf( "should send back, mulitcast addr: 0x%x, port: %d\n",sockin.sin_addr.s_addr, sockin.sin_port );
				if (pNetData->len > sizeof(struct NETDATA)+sizeof(NETWORK_LAN))
				{
					char s_data[64] = {0};
					strcpy( s_data, pNetData->data + sizeof(NETWORK_LAN) );
					printf(	"len= %d, get mac addr: %s\n", pNetData->len, s_data );

					Network_Config_Data net;
					get_mac( &net );
					char s_mac[32] = {0};
					sprintf( s_mac, "%02X:%02X:%02X:%02X:%02X:%02X", net.MAC[0],net.MAC[1],net.MAC[2],net.MAC[3],net.MAC[4],net.MAC[5] );
					if ( 0 == strcmp( s_mac, s_data) )
					{
						;//printf( "mac is same.\n");
					}
					else
					{
						sstw_error( "mac is't same.");
						continue;
					}
				}

				if (pNetData->type == SET_NETWORK_LAN)
				{
					ver = set_network_lan((void*)pNetData);
				}
				else
				{					
					ver = get_network_lan((void*)pNetData);
				}
				if ( ver == 5 )
				{
					printf( "should send back, mulitcast addr: 0x%x, port: %d\n",sockin.sin_addr.s_addr, sockin.sin_port );
					iRet = sendto(m_iSocket, (char*)pNetData, pNetData->len, 0, (struct sockaddr *)&sockin, SOCK_SIZE);
				}	
			}
			
			else if( pNetData->type == SET_REBOOT )
			{									
				set_reboot((void*)pNetData);
			}
			else if( pNetData->type == GET_DEVICE_INFO )
			{	
				ver = get_device_info( (void *) pNetData );
				if ( ver == 5 )
				{
					iRet = sendto(m_iSocket, (char*)pNetData, pNetData->len, 0, (struct sockaddr *)&sockin, SOCK_SIZE);
				}	
			}
			else
			{
				sstw_error( "Unknow command...%d", pNetData->type );
			}
		}
	}
		return NULL;
}

#include <netdb.h>

int init_lan_probe()
{
  //  m_iSocket = socket(AF_INET,SOCK_DGRAM,0,NULL,0,WSA_FLAG_MULTIPOINT_C_LEAF|WSA_FLAG_MULTIPOINT_D_LEAF|WSA_FLAG_OVERLAPPED);
    m_iSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);   // SOCK_STREAM
     //
     if (m_iSocket  == -1)
     {
		 perror( "socket error" );
         return -1;
     }

     struct sockaddr_in local;
	 
     local.sin_family = AF_INET;
	 
#ifdef MULIT_CASE
     local.sin_port = htons(MCASTPORT);
     local.sin_addr.s_addr = inet_addr( MCASTADDR );
#else
     local.sin_addr.s_addr = htonl( INADDR_ANY );
     local.sin_port = htons(LANSERVER_IPCAM_PORT);
#endif

     int tmp = 1;
     setsockopt(m_iSocket, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

     int iRet = bind( m_iSocket, (struct sockaddr *)&local, sizeof(local));
     if (iRet == -1 )
     {
         char msg[128] = {0};
         sprintf( msg, "LAN server:Bind port failed: %d \n", LANSERVER_IPCAM_PORT);
         printf(msg);

         return -1;
     }
	 
#ifdef MULIT_CASE
//     struct sockaddr_in remote;

//	remote.sin_family = AF_INET;
//	remote.sin_port = htons(MCASTPORT);
//	remote.sin_addr.s_addr = inet_addr( MCASTADDR );
	
	struct ip_mreq mreq;
	bzero(&mreq, sizeof(struct ip_mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(MCASTADDR);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (mreq.imr_multiaddr.s_addr == -1) {
    		printf("Error: group of 224.0.0.1 not a legal multicast address\n");
	}
	  
	if (setsockopt(m_iSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,sizeof(struct ip_mreq)) == -1)
	{
		perror( "IP_ADD_MEMBERSHIP error" );
		return -1;
	}	
#endif

     //
     if( pthread_create(&m_hThread, NULL, LANServer_thread, NULL) )
	 {
		 sstw_error("create lanserver_thread failed");
		 exit(0);
	 }

     printf("IP camera LAN server started!\n");

    return 0;
}

//
void CloseLANServer()
{
    if (-1 != m_iSocket)
    {
        shutdown(m_iSocket, SHUT_RDWR);
        m_iSocket = -1;
    }

    //
    if (-1 != m_hThread)
    {
        pthread_join(m_hThread, NULL);
        m_hThread = -1;
    }
}
