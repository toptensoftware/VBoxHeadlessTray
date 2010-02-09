//////////////////////////////////////////////////////////////////////////
// Utils.h - declaration of Utils

#ifndef __UTILS_H
#define __UTILS_H

BOOL WinExec(const wchar_t* pszCommandLine);

enum ManageAutoRunOp
{
	maroSet,
	maroClear,
	maroQuery,
};


bool ManageAutoRun(const wchar_t* pszAppName, ManageAutoRunOp op, const wchar_t* pszArgs);

BOOL SlxShutdownBlockReasonCreate(HWND hWnd, LPCWSTR pwszReason);
BOOL SlxShutdownBlockReasonDestroy(HWND hWnd);

void log_open(const char* psz);
void log_close();
void log(const char* psz, ...);

#endif	// __UTILS_H

