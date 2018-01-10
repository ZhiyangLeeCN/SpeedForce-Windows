#ifndef _LSP_MANAGER_API_H
#define _LSP_MANAGER_API_H

#include <windows.h>

#ifdef LM_API_EXPORTS
#define LM_API extern "C" __declspec(dllexport)
#define LM_CLASS_API __declspec(dllexport)
#else
#define LM_API extern "C" __declspec(dllimport)
#define LM_CLASS_API __declspec(dllimport)
#endif

#define LM_API_CALL _stdcall

LM_API int LM_API_CALL VerifySfLspIsInstalled();

LM_API int LM_API_CALL UninstallSfLsp();

LM_API int LM_API_CALL InstallSfLsp(LPWCH lpszLspPathAndFile, LPWCH lpszLspPathAndFile32);

#endif // !_LSP_MANAGER_API_H
