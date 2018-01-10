#include <stdio.h>

#include "lsp_manager_api.h"

// install 64-bit and 32-bit lsp
LPCWCH FLAG_INSTALL = TEXT("i");

// reinstall lsp
LPCWCH FLAG_REINSTALL = TEXT("r");

// unstall lsp
LPCWCH FLAG_UNSTALL = TEXT("u");

BOOL FileIsExist(LPCTSTR lpFileName)
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind;

	hFind = FindFirstFile(lpFileName, &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	else {
		FindClose(hFind);
		return TRUE;
	}
	
}

BOOL VerifyLsp(LPCTSTR lpFilePath)
{
	HMODULE hModule;
	void ( *lpGetLspGuidFunction )( LPGUID );

	hModule = LoadLibrary( lpFilePath );
	if (NULL == hModule) {
		wprintf(TEXT("Error:load lsp[%s] error, code:%d\n"), lpFilePath, GetLastError());
		return FALSE;
	}

	lpGetLspGuidFunction = (void(*)(LPGUID))GetProcAddress(hModule, "GetLspGuid");
	if (NULL == lpGetLspGuidFunction) {
		wprintf(TEXT("Error:this dll[%s] not lsp.\n"), lpFilePath);
		return FALSE;
	}

	FreeLibrary(hModule);
	return TRUE;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	if (argc < 3) {
		wprintf(TEXT("Error:The number of parameters is not enough.\n"));
		return -1;
	}

	LPWCH lpwszLspPathAndFile = argv[1]; //64-bit lsp dll file path
	LPWCH lpwszLspPathAndFile32 = argv[2]; //32-bit lsp dll file path
	LPWCH lpwszFlag = argv[3];
	int   rc;

	if (!FileIsExist(lpwszLspPathAndFile)) {
		wprintf(TEXT("Error:64-bit lsp file not exist.\n"));
		return -1;
	}
	if (!FileIsExist(lpwszLspPathAndFile32)) {
		wprintf(TEXT("Error:32-bit lsp file not exist.\n"));
		return -1;
	}

	//if (!VerifyLsp(lpwszLspPathAndFile) || !VerifyLsp(lpwszLspPathAndFile32)) {
	//	wprintf(TEXT("verify lsp fail.\n"));
	//	return - 1;
	//}

	if (0 == wcscmp(lpwszFlag, FLAG_INSTALL)) {
		
		if (VerifySfLspIsInstalled() == 0) {

			if (InstallSfLsp(lpwszLspPathAndFile, lpwszLspPathAndFile32) != 0) {
				wprintf(TEXT("Error: install lsp fail.\n"));
				return -1;
			}

		}

	} else if (0 == wcscmp(lpwszFlag, FLAG_REINSTALL)) {
		
		rc = UninstallSfLsp();
		if (rc < 0) {
			wprintf(TEXT("Error: uninstall lsp fail.\n"));
			return -1;
		}

		if (InstallSfLsp(lpwszLspPathAndFile, lpwszLspPathAndFile32) != 0) {
			wprintf(TEXT("Error: install lsp fail.\n"));
			return -1;
		}

	} else if (0 == wcscmp(lpwszFlag, FLAG_UNSTALL)) {

		rc = UninstallSfLsp();
		if (rc < 0) {
			wprintf(TEXT("Error: uninstall lsp fail.\n"));
			return -1;
		}

	} else {
		wprintf(TEXT("Error:unknown flag:%s\n"), lpwszFlag);
		return -1;
	}


	return 0;
}