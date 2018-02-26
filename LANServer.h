/*
 * LANServer.h
 *
 *  Created on: 2011-6-22
 *      Author: zhangxian
 */

#ifndef LANSERVER_H_
#define LANSERVER_H_

#include "stdafx.h"

#include "net_config.h"

#define LANSERVER_IPCAM_PORT                   6022

typedef unsigned char byte;

//
typedef enum _LAN_MSG_TYPE_
{
    LAN_SEARCH_DEVICE = 10,
    LAN_SEARCH_DEVICE2 = 11,

    LAN_SEARCH_DEVICE_ACK = 100,
    LAN_SEARCH_DEVICE2_ACK = 101,

}LAN_MSG_TYPE;

//
int init_lan_probe();
//
void CloseLANServer();



#endif /* LANSERVER_H_ */
