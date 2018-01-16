#ifndef _SOCKS_V5_H_
#define _SOCKS_V5_H_

#include <winsock2.h>
#include <ws2spi.h>

#define SVERSION 0x05
#define CONNECT 0x01
#define IPV4 0x01
#define DOMAIN 0x03
#define IPV6 0x04
#define METHOD_NOAUTH 0x00
#define METHOD_UNACCEPTABLE 0xff
#define CMD_NOT_SUPPORTED 0x07

#pragma pack(1)

typedef struct _METHOD_SELECT_REQUEST {
	unsigned char ver;
	unsigned char nmethods;
	unsigned char methods[0];
} METHOD_SELECT_REQUEST;

typedef struct _METHOD_SELECT_RESPONSE {
	unsigned char ver;
	unsigned char method;
} METHOD_SELECT_RESPONSE;

typedef struct _SOCKS5_REQUEST {
	unsigned char ver;
	unsigned char cmd;
	unsigned char rsv;
	unsigned char atyp;
	unsigned char addr[0];
} SOCKS5_REQUEST;

typedef struct _SOCKS5_RESPONSE {
	unsigned char ver;
	unsigned char rep;
	unsigned char rsv;
	unsigned char atyp;
} SOCKS5_RESPONSE;

#pragma pack()

BOOL IsNeedProxy(SOCKADDR *addr);

int SockerV5TCPHandshake(
		SOCKET			s,
		SOCKADDR		*socksServerAddr,
		SOCKADDR		*destAddr,
		int				*lpErrorno
	);

#endif // !_SOCKS_V5_H_

