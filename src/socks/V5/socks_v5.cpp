#include "socks_v5.h"

#define REV_BUFF_LEN 1024

BOOL
IsNeedProxy(SOCKADDR *addr)
{
	if (addr->sa_family == AF_INET) {

		SOCKADDR_IN *inetAddr = (SOCKADDR_IN *)addr;
		UCHAR net = inetAddr->sin_addr.s_net;
		UCHAR host = inetAddr->sin_addr.s_host;

		if (net == 0 || inetAddr->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
			return FALSE;
		if (net > 233 || net == 10)
			return FALSE;
		if (net == 172 && (host >= 16 && host <= 32))
			return FALSE;
		if (net == 192 && host == 168)
			return FALSE;

		return TRUE;

	}

	return FALSE;
}

int SockerV5TCPHandshake(
	SOCKET			s, 
	SOCKADDR		*socksServerAddr, 
	SOCKADDR		*destAddr,
	int				*lpErrorno
)
{
	METHOD_SELECT_REQUEST  *lpMethodSelectRequest = NULL;
	METHOD_SELECT_RESPONSE *lpMethodSelectResponse = NULL;
	SOCKS5_REQUEST         *lpSocks5Request = NULL;
	SOCKS5_RESPONSE		   *lpSocks5Reponse = NULL;
	char				   *lpSendBuff,
						   lpRecvBuff[REV_BUFF_LEN],
						   *lpszIpv4;
	int					   rc;


	lpMethodSelectRequest = (METHOD_SELECT_REQUEST *)malloc(sizeof(METHOD_SELECT_REQUEST) + 1);
	lpMethodSelectRequest->ver = SVERSION;
	lpMethodSelectRequest->nmethods = 1;
	lpMethodSelectRequest->methods[0] = METHOD_NOAUTH;

	lpSendBuff = (char *)lpMethodSelectRequest;
	rc = send(s, lpSendBuff, sizeof(METHOD_SELECT_REQUEST) + 1, 0);
	if (SOCKET_ERROR == rc) {
		goto cleanup;
	}

	rc = recv(s, lpRecvBuff, REV_BUFF_LEN, 0);
	if (0 == rc || SOCKET_ERROR == rc) {
		rc = SOCKET_ERROR;
		goto cleanup;
	}

	lpMethodSelectResponse = (METHOD_SELECT_RESPONSE *)lpRecvBuff;
	if (METHOD_UNACCEPTABLE == lpMethodSelectResponse->method) {
		rc = -1;
		goto cleanup;
	}


	lpSocks5Request = (SOCKS5_REQUEST *)malloc(
		sizeof(SOCKS5_REQUEST) + 
		4/*IPv4 len*/ + 
		2/*port len*/
	);
	lpSocks5Request->ver = SVERSION;
	lpSocks5Request->cmd = CONNECT;
	lpSocks5Request->rsv = 0x00;
	lpSocks5Request->atyp = IPV4;
	::memcpy(lpSocks5Request->addr, &((SOCKADDR_IN *)destAddr)->sin_addr, 4);
	::memcpy((lpSocks5Request->addr + 4), &((SOCKADDR_IN *)destAddr)->sin_port, 2);

	lpSendBuff = (char *)lpSocks5Request;
	rc = send(s, lpSendBuff, sizeof(SOCKS5_REQUEST) + 4 + 2, 0);
	if (SOCKET_ERROR == rc) {
		goto cleanup;
	}

	rc = recv(s, lpRecvBuff, REV_BUFF_LEN, 0);
	if (0 == rc || SOCKET_ERROR == rc) {
		rc = SOCKET_ERROR;
		goto cleanup;
	}

	lpSocks5Reponse = (SOCKS5_RESPONSE *)lpRecvBuff;
	if (0x00 != lpSocks5Reponse->rep) {
		rc = -1;
		goto cleanup;
	}

cleanup:

	if (NULL != lpMethodSelectRequest) {
		free(lpMethodSelectRequest);
	}

	if (NULL != lpSocks5Request) {
		free(lpSocks5Request);
	}

	if (SOCKET_ERROR == rc) {
		*lpErrorno = WSAGetLastError();
		return rc;
	}

	return 0;
}
