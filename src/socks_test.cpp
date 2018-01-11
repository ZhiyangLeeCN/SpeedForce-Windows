#include "socks\V5\socks_v5.h"
#include <stdio.h>

int main()
{
	WSADATA wsData;
	WSAStartup(MAKEWORD(2, 2), &wsData);

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in socksServerAddr;
	socksServerAddr.sin_family = AF_INET;
	socksServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	socksServerAddr.sin_port = htons(1080);

	sockaddr_in destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	destAddr.sin_port = htons(8123);

	int lpErrorno;

	int rc = ConnectSocksV5ServerForTcp(
		s, 
		(SOCKADDR *)&socksServerAddr, 
		(SOCKADDR *)&destAddr, 
		&lpErrorno
	);

	if (rc == 0) {
		printf("CONNECT SUCCESS\n");
		send(s, "Hi", 3, 0);
	}
	else {
		printf("CONNECT ERROR\n");
		closesocket(s);
	}

	getchar();
	return 0;
}