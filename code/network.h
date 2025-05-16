#ifndef NETWORK_H
#define NETWORK_H

#include "xparameters.h"
#include "netif/xadapter.h"
#include "lwip/init.h"
#include "platform_config.h"
#include "xil_printf.h"

/* --------------------------------------------------*/
// TODO: Set appropriate values for IP Address, Gateway and Netmask
// IP address settings
#define ADDR1 169
#define ADDR2 254
#define ADDR3 60
#define ADDR4 205
/* --------------------------------------------------*/

// Gateway address settings
#define GW1 255
#define GW2 255
#define GW3 255
#define GW4 255

// Netmask settings
#define NETMASK1 255
#define NETMASK2 255
#define NETMASK3 0
#define NETMASK4 0

#define THREAD_STACKSIZE 	1024
#define START_DELAY 		1000

// useful for printing the defined values
#define PRINT_IP(msg, ip) \
	xil_printf(msg); \
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),ip4_addr3(ip), ip4_addr4(ip));

int main_thread();
void network_thread(void *p);
void print_ip_setup(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);

#endif