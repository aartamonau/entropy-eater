/**
 * @file   eater_server.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sat Sep 11 17:45:07 2010
 *
 * @brief  Routines to expose entropy eater to user space through generic
 * netlink sockets.
 *
 */

#ifndef _EATER_SERVER_H_
#define _EATER_SERVER_H_


#include <net/genetlink.h>

#include "eater_interface.h"


/**
 * Makes entropy eater available through generic netlink socket interface.
 *
 * @return status
 * @retval  0 success
 * @retval <0 error code
 */
int
eater_server_register(void);


/**
 * Unregisters entropy eater server.
 *
 *
 * @return status
 * @retval  0 success
 * @retval <0 error code
 */
int
eater_server_unregister(void);


#endif /* _EATER_SERVER_H_ */
