#ifndef _LSP_MANAGER_API_H
#define _LSP_MANAGER_API_H

#ifdef LM_API_EXPORTS
#define LM_API extern "C" __declspec(dllexport)
#define LM_CLASS_API __declspec(dllexport)
#else
#define LM_API __declspec(dllimport)
#define LM_CLASS_API __declspec(dllimport)
#endif

LM_API int test();

LM_API int VerifyLspIsInstalled();

#endif // !_LSP_MANAGER_API_H
