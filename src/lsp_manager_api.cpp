#include "instlsp.h"
#include "lspguid.h"
#include <windows.h>

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

#define REQUIRE_PROTOCOL_LEN 2

const REQUIRE_PROTOCOL_INFO gRequireProtocolInfos[REQUIRE_PROTOCOL_LEN] = {
	{AF_INET, SOCK_STREAM, IPPROTO_TCP},//IP4|TCP
	{AF_INET, SOCK_DGRAM, IPPROTO_UDP},//IP4|UDP
	//{AF_INET, SOCK_RAW, IPPROTO_RAW}//IP4|RAW
};

LPWSCUPDATEPROVIDER  fnWscUpdateProvider,
fnWscUpdateProvider32;

LM_API 
int LM_API_CALL VerifySfLspIsInstalled()
{
	LPWSAPROTOCOL_INFOW pProtocolInfo = NULL;
	DWORD				dummyLspId = 0;
	INT					iTotalProtocols = 0;
	int					count = 0, 
						i = 0, 
						j = 0;

	pProtocolInfo = EnumerateProviders(gCatalog, &iTotalProtocols);
	if (NULL == pProtocolInfo) {
		dbgprint(
			"MS_API::VerifyLspIsInstalled Error: Unable to enumerate Winsock catalog\n"
		);
		return -1;
	}

	for (i = 0; i < iTotalProtocols; i++) {
		if (0 == memcmp(&pProtocolInfo[i].ProviderId, &gProviderGuid, sizeof(gProviderGuid))) {
			dummyLspId = pProtocolInfo[i].dwCatalogEntryId;
			break;
		}
	}

	if (0 == dummyLspId) {
		goto cleanup;
	}
	
	count = 0;
	for (i = 0; i < iTotalProtocols; i++) {

		if (pProtocolInfo[i].ProtocolChain.ChainLen > 1 &&
			pProtocolInfo[i].ProtocolChain.ChainEntries[0] == dummyLspId) {
			count++;
		} else if(pProtocolInfo[i].dwCatalogEntryId == dummyLspId) {
			count++;
		}

	}

cleanup:

	if (NULL != pProtocolInfo) {
		FreeProviders(pProtocolInfo);
		pProtocolInfo = NULL;
	}

	return count;
}

LM_API
int LM_API_CALL UninstallSfLsp()
{
	LPWSAPROTOCOL_INFOW pProtocolInfo = NULL;
	DWORD				dummyLspId = 0;
	INT					iTotalProtocols = 0;
	int					i = 0, 
						rc = 0;

	pProtocolInfo = EnumerateProviders(gCatalog, &iTotalProtocols);
	if (NULL == pProtocolInfo) {
		dbgprint(
			"MS_API::UninstallLsp Error: Unable to enumerate Winsock catalog\n"
		);
		rc = -1;
		goto cleanup;
	}

	for (i = 0; i < iTotalProtocols; i++) {
		if (0 == memcmp( &pProtocolInfo[i].ProviderId, &gProviderGuid, sizeof( gProviderGuid ) )) {
			dummyLspId = pProtocolInfo[i].dwCatalogEntryId;
			DeinstallProvider(gCatalog, &pProtocolInfo[i].ProviderId);
			rc++;
			break;
		}
	}

	if (dummyLspId > 0) {

		for ( i = 0; i < iTotalProtocols; i++)
		{
			if ( ( pProtocolInfo[i].dwCatalogEntryId == dummyLspId ) || 
				( pProtocolInfo[i].ProtocolChain.ChainLen > 1 &&
				pProtocolInfo[i].ProtocolChain.ChainEntries[0] == dummyLspId ) ) {
				DeinstallProvider(gCatalog, &pProtocolInfo[i].ProviderId);
				rc++;
			}
		}

	} else {


		WCHAR wszLspDefaultName[ WSAPROTOCOL_LEN ];
		rc = MultiByteToWideChar(
			CP_ACP,
			0,
			DEFAULT_LSP_NAME,
			( int ) strlen(DEFAULT_LSP_NAME) + 1,
			wszLspDefaultName,
			WSAPROTOCOL_LEN
			);

		if (0 == rc) {
			fprintf(stderr, "UninstallSfLsp: MultiByteToWideChar failed to convert '%s'; Error = %d\n",
				DEFAULT_LSP_NAME, GetLastError());
			goto cleanup;
		}

		rc = 0;
		for ( i = 0; i < iTotalProtocols; i++ ) {

			if ( NULL != wcsstr(pProtocolInfo[i].szProtocol, wszLspDefaultName) ) {
				DeinstallProvider(gCatalog, &pProtocolInfo[i].ProviderId);
				rc++;
			}

		}

	}

cleanup:

	if (NULL != pProtocolInfo) {
		FreeProviders(pProtocolInfo);
		pProtocolInfo = NULL;
	}

	return rc;
}

LM_API
int LM_API_CALL InstallSfLsp(LPWCH lpszLspPathAndFile, LPWCH lpszLspPathAndFile32)
{
	LPWSAPROTOCOL_INFOW pProtocolInfo = NULL;
	DWORD				*pdwCatalogIdArray = NULL, 
						dwCatalogIdArrayCount = 0,
						dwCatalogIdArrayIndex = 0;
	INT					iTotalProtocols = 0;
	int					lpErron, 
						i = 0, 
						j = 0,
						rc = 0;

	pProtocolInfo = EnumerateProviders(gCatalog, &iTotalProtocols);
	if (NULL == pProtocolInfo) {
		dbgprint(
			"MS_API::InstallLsp Error: Unable to enumerate Winsock catalog\n"
		);
		rc = -1;
		goto cleanup;
	}

	for (i = 0; i < REQUIRE_PROTOCOL_LEN; i++)
	{
		for (j = 0; j < iTotalProtocols; j++)
		{
			if (BASE_PROTOCOL == pProtocolInfo[j].ProtocolChain.ChainLen) {

				if (gRequireProtocolInfos[i].af == pProtocolInfo[j].iAddressFamily && 
					gRequireProtocolInfos[i].type == pProtocolInfo[j].iSocketType &&
					gRequireProtocolInfos[i].protocol == pProtocolInfo[j].iProtocol
					) {
					dwCatalogIdArrayCount++;
				}

			}
		}
	}

	pdwCatalogIdArray = (DWORD*)LspAlloc(dwCatalogIdArrayCount * sizeof(DWORD), &lpErron);
	if (NULL == pdwCatalogIdArray) {
		dbgprint(
			"MS_API::InstallLsp Error:  LspAlloc failed: %d\n", lpErron
		);
		rc = -1;
		goto cleanup;
	}

	for (i = 0; i < REQUIRE_PROTOCOL_LEN; i++)
	{
		for (j = 0; j < iTotalProtocols; j++)
		{
			if (BASE_PROTOCOL == pProtocolInfo[j].ProtocolChain.ChainLen) {

				if (gRequireProtocolInfos[i].af == pProtocolInfo[j].iAddressFamily &&
					gRequireProtocolInfos[i].type == pProtocolInfo[j].iSocketType &&
					gRequireProtocolInfos[i].protocol == pProtocolInfo[j].iProtocol
					) {

					pdwCatalogIdArray[dwCatalogIdArrayIndex++] = pProtocolInfo[j].dwCatalogEntryId;
				}

			}
		}
	}

	rc = InstallLsp(
		gCatalog,
		TEXT(DEFAULT_LSP_NAME),
		lpszLspPathAndFile,
		lpszLspPathAndFile32,
		dwCatalogIdArrayCount,
		pdwCatalogIdArray,
		FALSE,
		FALSE
	);
	

cleanup:

	if (NULL != pProtocolInfo) {
		FreeProviders(pProtocolInfo);
		pProtocolInfo = NULL;
	}

	if (NULL != pdwCatalogIdArray) {
		LspFree(pdwCatalogIdArray);
		pdwCatalogIdArray = NULL;
	}

	return rc;
}
