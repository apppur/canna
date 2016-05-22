#ifndef _SOCKET_DEFINE_H
#define _SOCKET_DEFINE_H

#define SOCKET_DATA 0
#define SOCKET_CLOSE 1
#define SOCKET_OPEN 2
#define SOCKET_ACCEPT 3
#define SOCKET_ERROR 4
#define SOCKET_EXIT 5
#define SOCKET_UDP 6

#define MAX_INFO 128
#define MAX_SOCKET_P 16
#define MAX_SOCKET (1<<MAX_SOCKET_P)
#define MAX_EVENT 64

#define HASH_ID(id) (((unsigned)id) % MAX_SOCKET)

#define SOCKET_TYPE_INVALID 0
#define SOCKET_TYPE_RESERVE 1
#define SOCKET_TYPE_PLISTEN 2

#define PROTOCOL_TCP 0
#define PROTOCOL_UDP 1
#define PROTOCOL_UDPv6 2

#endif
