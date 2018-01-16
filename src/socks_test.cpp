#include "socks\V5\socks_v5.h"
#include <stdio.h>

int ioTest(SOCKET s, SOCKADDR *destAddr)
{
	int rc = 0;

	rc = send(s, "Hi\n", 4, 0);
	if (SOCKET_ERROR == rc) {
		printf("SEND ERROR:%d\n", WSAGetLastError());
	}
	else {
		rc = 0;
		printf("SEND OK :%d\n");
	}

	char recvBuff[4096];
	rc = recv(s, recvBuff, 4096, 0);
	if (rc == SOCKET_ERROR) {
		printf("recv error\n");
	}
	else {
		rc = 0;
		printf("recv ok:%s\n", recvBuff);
	}

	return 0;

}

int enventSelectTest(SOCKET s)
{
	u_long enable = 1, disabled = 0;

	int nEventTotal = 0;
	int rc = 0;

	WSAEVENT    eventArray[WSA_MAXIMUM_WAIT_EVENTS];
	SOCKET      sockArray[WSA_MAXIMUM_WAIT_EVENTS];

	WSAEVENT eventHanlde = WSACreateEvent();
	WSAEventSelect(s, eventHanlde, FD_CONNECT | FD_CLOSE);

	eventArray[nEventTotal] = eventHanlde;
	sockArray[nEventTotal] = s;
	nEventTotal++;

	BOOL isLoop = TRUE;
	while (isLoop) {

		rc = WSAWaitForMultipleEvents(nEventTotal, eventArray, FALSE, WSA_INFINITE, FALSE);
		if (rc == WSA_WAIT_FAILED || rc == WSA_WAIT_TIMEOUT) {
			printf("enventSelectTest::WSAWaitForMultipleEvents Error:%d\n", WSAGetLastError());
			rc = SOCKET_ERROR;
		}

		size_t lastIndex = rc - WSA_WAIT_EVENT_0;
		for (size_t i = lastIndex; i < nEventTotal; i++)
		{
			rc = WSAWaitForMultipleEvents(1, &eventArray[i], TRUE, 1000, FALSE);
			if (rc == WSA_WAIT_FAILED || rc == WSA_WAIT_TIMEOUT) {
				continue;
			}

			WSANETWORKEVENTS event;
			rc = WSAEnumNetworkEvents(sockArray[i], eventArray[i], &event);
			if (rc == SOCKET_ERROR) {
				printf("WSAEnumNetworkEvents Error:%d\n", WSAGetLastError());
				isLoop = FALSE;
				break;
			}

			if (event.lNetworkEvents & FD_CONNECT) {

				WSAEVENT newEvent = WSACreateEvent();
				WSAEventSelect(s, newEvent, FD_READ | FD_CLOSE | FD_WRITE);
				eventArray[nEventTotal] = newEvent;
				sockArray[nEventTotal] = s;
				nEventTotal++;
				printf("connect ok\n");

			}

			if (event.lNetworkEvents & FD_WRITE) {
				rc = send(s, "Hi\n", 4, 0);
				if (rc == SOCKET_ERROR) {
					printf("send error\n");
				}
				else {
					rc = 0;
					printf("send ok\n");
				}
			}

			if (event.lNetworkEvents & FD_READ) {
				isLoop = FALSE;
				char recvBuff[4096];
				rc = recv(s, recvBuff, 4096, 0);
				if (rc == SOCKET_ERROR) {
					printf("recv error\n");
				}
				else {
					recvBuff[rc] = '\0';
					rc = 0;
					printf("recv ok:%s\n", recvBuff);
				}
			}

			if (event.lNetworkEvents & FD_CLOSE) {
				printf("connection close\n");
				isLoop = FALSE;
			}

			if (rc == SOCKET_ERROR) {
				isLoop = FALSE;
			}

		}

	}

	return rc;
}

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
	destAddr.sin_port = htons(8124);

	int lpErrorno;

	//int rc = ConnectSocksV5ServerForTcp(
	//	s, 
	//	(SOCKADDR *)&socksServerAddr, 
	//	(SOCKADDR *)&destAddr, 
	//	&lpErrorno
	//);

	int rc = connect(s, (SOCKADDR *)&socksServerAddr, sizeof(socksServerAddr));

	if (0 != rc) {
		printf("connect error.\n");
	} else {
		int error = 0;
		rc = SockerV5TCPHandshake(s, (SOCKADDR *)&socksServerAddr, (SOCKADDR *)&destAddr, &error);
		if (0 != rc) {
			printf("SockerV5TCPHandshake error.\n");
		}
		else {
			enventSelectTest(s);
		}
	}

	closesocket(s);

	getchar();
	return 0;
}