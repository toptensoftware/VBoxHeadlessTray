//////////////////////////////////////////////////////////////////////////
// Utils.cpp - implementation of Utils

#include "stdafx.h"
#include "VBoxHeadlessTray.h"

#include "Utils.h"

#include <share.h>

BOOL WinExec(const wchar_t* pszCommandLine)
{
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb=sizeof(si);
	PROCESS_INFORMATION pi;
	if (CreateProcess(NULL, CUniString(pszCommandLine).GetBuffer(0), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return TRUE;
	}

	return FALSE;


}

bool ManageAutoRun(const wchar_t* pszAppName, ManageAutoRunOp op, const wchar_t* pszArgs)
{
	const wchar_t* pszRunKey=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	CUniString strCommandLine=Format(L"\"%s\"", SlxGetModuleFileName(NULL));
	if (pszArgs)
	{
		strCommandLine+=L" ";
		strCommandLine+=pszArgs;
	}

	switch (op)
	{
		case maroSet:
			return RegSetString(HKEY_CURRENT_USER, pszRunKey, pszAppName, strCommandLine)==ERROR_SUCCESS;

		case maroClear:
		{
			CRegKey key;
			if (key.Open(HKEY_CURRENT_USER, pszRunKey, KEY_READ|KEY_WRITE)!=ERROR_SUCCESS)
				return false;

			return RegDeleteValue(key, pszAppName)==ERROR_SUCCESS;
		}

		case maroQuery:
		{
			CUniString str;
			if (RegGetString(HKEY_CURRENT_USER, pszRunKey, pszAppName, str)!=ERROR_SUCCESS)
				return false;

			return IsEqualString(str, strCommandLine);
		}
	}

	return false;

}


typedef BOOL (WINAPI* PFNSHUTDOWNBLOCKREASONCREATE)(
    HWND hWnd,
    LPCWSTR pwszReason);

typedef BOOL (WINAPI* PFNSHUTDOWNBLOCKREASONDESTROY)(
    __in HWND hWnd);


BOOL SlxShutdownBlockReasonCreate(HWND hWnd, LPCWSTR pwszReason)
{
	PFNSHUTDOWNBLOCKREASONCREATE pfn=(PFNSHUTDOWNBLOCKREASONCREATE)GetProcAddress(GetModuleHandle(L"user32.dll"), "ShutdownBlockReasonCreate");
	if (pfn)
		return pfn(hWnd, pwszReason);
	else
		return FALSE;
}


BOOL SlxShutdownBlockReasonDestroy(HWND hWnd)
{
	PFNSHUTDOWNBLOCKREASONDESTROY pfn=(PFNSHUTDOWNBLOCKREASONDESTROY)GetProcAddress(GetModuleHandle(L"user32.dll"), "ShutdownBlockReasonDestroy");
	if (pfn)
		return pfn(hWnd);
	else
		return FALSE;
}

class CLogFile
{
public:
	CLogFile()
	{
		m_pLog=stdout;
	}

	~CLogFile()
	{
		Close();
	}

	errno_t Open(const char* psz)
	{
		Close();
		m_pLog=_fsopen(psz, "wt", _SH_DENYWR);
		if (!m_pLog)
		{
			SlxMessageBox(Format(L"Failed to open log file '%S' - error %i", psz, errno), MB_OK|MB_ICONINFORMATION);
		}
		return errno;
	}
	
	void Close()
	{
		if (m_pLog && m_pLog!=stdout && m_pLog!=stderr)
		{
			fclose(m_pLog);
			m_pLog=NULL;
		}
	}

	bool IsOpen()
	{
		return m_pLog!=NULL;
	}

	FILE* GetFile()
	{
		return m_pLog ;
	}

protected:
	FILE* m_pLog;

} g_log;

void log_open(const char* psz)
{
	// Open log file
	g_log.Open(psz);
}

void log_close()
{
	g_log.Close();
}

void log(const char* psz, ...)
{
	if (!g_log.IsOpen())
		return;

    va_list  args;
    va_start(args, psz);
	CString<char> str=Format(psz, args);
	va_end(args);

	SYSTEMTIME st;
	GetLocalTime(&st);

	fprintf(g_log.GetFile(), "%.2i/%.2i/%.4i %.2i:%.2i:%.2i.%.3i %s", 
						st.wDay, st.wMonth, st.wYear,
						st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
						str);
	fflush(g_log.GetFile());
}
