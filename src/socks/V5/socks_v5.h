#ifndef _SOCKS_V5_H_
#define _SOCKS_V5_H_

#include <winsock2.h>

int ConnectSocksV5Server(
		SOCKET   s,
		SOCKADDR *socksServerAddr,
		SOCKADDR *destAddr,
		LPWCH    userName,
		LPWCH    password
	);

#endif // !_SOCKS_V5_H_

