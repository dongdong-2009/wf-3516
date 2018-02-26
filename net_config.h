/* ===========================================================================
* @path $(IPNCPATH)\sys_adm\system_server
*
* @desc 
* .
* Copyright (c) Appro Photoelectron Inc.  2008
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied
*
* =========================================================================== */
/**
* @file net_config.h
* @brief Functions about network settings.
*/

#ifndef _NET_CONFIG_H_
#define _NET_CONFIG_H_

#include <paths.h>
#include "common2.h"

#define DHCPC_EXEC				"dhcpcd"
#define DHCPC_EXEC_PATH			"/root/ipcam/"DHCPC_EXEC
//#define DHCPC_EXEC_PATH			"udhcpc"
#define DHCPC_PID_FILE_PATH		""_PATH_VARRUN"dhcpcd-%s.pid"
#ifdef hisilicon
#define RESOLV_CONF				"/mnt/mtd/etc/resolv.conf"
#else
#define RESOLV_CONF				"/etc/resolv.conf"
#endif
//#define RESOLV_CONF				"/var/dhcpc/resolv.conf"
#define PROCNET_ROUTE_PATH		"/proc/net/route"

#define MAC_FILE "/mnt/mtd/etc/mac"
#define NETWORK_FILE "/mnt/mtd/etc/network.cfg"

//gv mac id
#define MAC_GV_0 0xE0
#define MAC_GV_1 0xAA
#define MAC_GV_2 0xB0

//zhongsh mac 
#define MAC_COMPANY_0 0x02
#define MAC_COMPANY_1 0x12
#define MAC_COMPANY_2 0x00

/**
* @brief Structure to save network status
*/
struct NET_CONFIG {
	unsigned char mac[6];
	in_addr_t	ifaddr;
	in_addr_t	netmask;
	in_addr_t	gateway;
	in_addr_t	dns;
};


//int net_set_flag(char *ifname, short flag);
int net_nic_up(char *ifname);
//int net_clr_flag(char *ifname, short flag);
int net_get_flag(char *ifname);
int net_nic_down(char *ifname);
in_addr_t net_get_ifaddr(char *ifname);
int net_set_ifaddr(char *ifname, in_addr_t addr);
in_addr_t net_get_netmask(char *ifname);
int net_set_netmask(char *ifname, in_addr_t addr);
int net_get_hwaddr(char *ifname, unsigned char *mac);
in_addr_t net_get_gateway(void);
int net_set_gateway(in_addr_t addr);
int net_clean_gateway(void);
//int net_add_gateway(in_addr_t addr);
//int net_del_gateway(in_addr_t addr);
in_addr_t net_get_dns(void);
int net_set_dns(char *dnsname);
pid_t net_start_dhcpcd(char *ifname);
int net_renew_dhcpcd(pid_t pid);
void net_enable_dhcpcd(void);
void net_disable_dhcpcd(void);
int net_get_info(char *ifname, Network_Config_Data *netcfg);
//struct in_addr net_get_ip(char *ifname);
//int network_init( Network_Config_Data* net );
int save_mac( char * mac_addr );

int get_mac( Network_Config_Data* net );
int network_set_mac( Network_Config_Data* net );

#endif