/**
 * @file   eater_interface.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sat Sep 11 17:18:55 2010
 *
 * @brief  Declarations related to kernel to user space interface of entropy
 * eater.
 *
 */

#ifndef _EATER_INTERFACE_H_
#define _EATER_INTERFACE_H_


/// Protocol name to be used for communications with entropy eater.
#define EATER_PROTO_NAME "eater"


/// Protocol version.
#define EATER_PROTO_VERSION 0


/// Entropy eater's attributes.
enum eater_attr_t {
  EATER_ATTR_NONE,              /**< For calls with no arguments. */
  __EATER_ATTR_MAX,
};


/// Maximum valid attribute.
#define EATER_ATTR_MAX (__EATER_ATTR_MAX - 1)


/// Commands that are supported by entropy eater.
enum eater_cmd_t {
  EATER_CMD_HELLO,                /**< Says hello to entropy eater. */
  __EATER_CMD_MAX
};


/// Maximum valid command.
#define EATER_CMD_MAX (__EATER_CMD_MAX - 1)


#endif /* _EATER_INTERFACE_H_ */
