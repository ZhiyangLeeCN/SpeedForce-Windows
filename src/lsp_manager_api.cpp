#include "instlsp.h"
#include "lspguid.h"

#define LM_API_EXPORTS

#include "lsp_manager_api.h"

#ifdef _WIN64
const WINSOCK_CATALOG     gCatalog = LspCatalog64Only;
#else
const WINSOCK_CATALOG     gCatalog = LspCatalog32Only;
#endif

typedef struct _REQUIRE_PROTOCOL_INFO {
	int af;
	int type;
	int	protocol;
} REQUIRE_PROTOCOL_INFO;

#define REQUIRE_PROTOCOL_LEN 3

//需要拦截的3个协议
const REQUIRE_PROTOCOL_INFO gRequireProtocolInfos[REQUIRE_PROTOCOL_LEN] = {
	{AF_INET, SOCK_STREAM, IPPROTO_TCP},//IP4|TCP
	{AF_INET, SOCK_DGRAM, IPPROTO_UDP},//IP4|UDP
	{AF_INET, SOCK_RAW, IPPROTO_RAW}//IP4|RAW
};

LM_API int test()
{
	return 201814;
}

LM_API 
int VerifyLspIsInstalled()
{
	LPWSAPROTOCOL_INFOW pProtocolInfo = NULL;
	INT iTotalProtocols = 0;
	int count, i, j = 0;

	pProtocolInfo = EnumerateProviders(gCatalog, &iTotalProtocols);
	if (NULL == pProtocolInfo) {
		dbgprint(
			"MS_API::VerifyLspIsInstalled Error: Unable to enumerate Winsock catalog\n"
		);
		return -1;
	}
	
	count = 0;
	for (i = 0; i < REQUIRE_PROTOCOL_LEN; i++) {

		for (j = 0; j < iTotalProtocols; j++) {

			if (gRequireProtocolInfos[i].af == pProtocolInfo[j].iAddressFamily && 
				gRequireProtocolInfos[i].type == pProtocolInfo[j].iSocketType && 
				gRequireProtocolInfos[i].protocol == pProtocolInfo[j].iProtocol) {
				count++;
			}

		}

	}

	FreeProviders( pProtocolInfo );
	pProtocolInfo = NULL;

	return count;
}
