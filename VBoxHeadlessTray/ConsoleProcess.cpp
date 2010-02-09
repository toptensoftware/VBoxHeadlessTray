//////////////////////////////////////////////////////////////////////////
// ConsoleProcess.cpp - implementation of ConsoleProcess

#include "stdafx.h"
#include "VBoxHeadlessTray.h"

#include "ConsoleProcess.h"

bool MakeHandleUninheritable(HANDLE& hHandle)
{
	HANDLE hTemp;
	if (!DuplicateHandle(GetCurrentProcess(), hHandle, GetCurrentProcess(), &hTemp, 0, FALSE, DUPLICATE_SAME_ACCESS))
		return false;

	CloseHandle(hHandle);
	hHandle=hTemp;
	return true;
}

CProcessPipes::CProcessPipes()
{
}

CProcessPipes::~CProcessPipes()
{
	Close();
}

void CProcessPipes::Close()
{
	CloseParentHandles();
	CloseChildHandles();
}

void CProcessPipes::CloseParentHandles()
{
	ATLTRACE(_T("Closing Parent Handles\n"));
	m_hPipeInputWrite.Release();
	m_hPipeOutputRead.Release();
	m_hPipeErrorRead.Release();
	ATLTRACE(_T("Closed Parent Handles\n"));
}

void CProcessPipes::CloseChildHandles()
{
	ATLTRACE(_T("Closing Child Handles\n"));
	m_hPipeInputRead.Release();
	m_hPipeOutputWrite.Release();
	m_hPipeErrorWrite.Release();
	ATLTRACE(_T("Closed Child Handles\n"));
}

bool CProcessPipes::Open(bool bErrorToOutput)
{
	ATLTRACE(_T("Opening Handles\n"));
	Close();
	if (!OpenInternal(bErrorToOutput))
	{
		Close();
		return false;
	}
	return true;
}

bool CProcessPipes::OpenInternal(bool bErrorToOutput)
{
	// Set up the security attributes struct.
	SECURITY_ATTRIBUTES sa;
	sa.nLength= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	// Create child output pipe
	if (!CreatePipe(&m_hPipeOutputRead, &m_hPipeOutputWrite,&sa,0))
		return false;

	if (!bErrorToOutput)
	{
		// Create child error pipe
		if (!CreatePipe(&m_hPipeErrorRead, &m_hPipeErrorWrite, &sa, 0))
			return false;
	}
	else
	{
		if (!DuplicateHandle(GetCurrentProcess(), m_hPipeOutputWrite, GetCurrentProcess(), &m_hPipeErrorWrite, 0, TRUE,DUPLICATE_SAME_ACCESS))
			return false;
	}

	// Create child input pipe
	if (!CreatePipe(&m_hPipeInputRead, &m_hPipeInputWrite, &sa, 0))
		return false;

	MakeHandleUninheritable(m_hPipeOutputRead.m_hObject);
	if (m_hPipeErrorRead)
		MakeHandleUninheritable(m_hPipeErrorRead.m_hObject);
	MakeHandleUninheritable(m_hPipeInputWrite.m_hObject);

	return true;
}


CPipeReader::CPipeReader()
{
}
CPipeReader::~CPipeReader()
{
	Close();
}

bool CPipeReader::Start(HANDLE hPipe, IPipeCallback* pCallback)
{
	ASSERT(m_hHandle==NULL);

	m_hPipe=hPipe;
	m_pCallback=pCallback;
	return CThread::Start();
}

void CPipeReader::Close()
{
	if (m_hHandle)
	{
		Wait(INFINITE);
		CThread::Close();
		m_hPipe=NULL;
		m_pCallback=NULL;
	}
}

DWORD CPipeReader::ThreadProc()
{
	ATLTRACE(_T("Pipe reader proc running %i\n"), GetCurrentThreadId());
	while (true)
	{
		char szBuffer[256];
		DWORD nBytesRead;

		if (!ReadFile(m_hPipe,szBuffer, sizeof(szBuffer), &nBytesRead,NULL) || !nBytesRead)
		{
			break;
		}

		m_pCallback->OnPipeRead(this, szBuffer, nBytesRead);
	}

	ATLTRACE(_T("Pipe reader proc closing %i\n"), GetCurrentThreadId());
	return 0;
}


CConsoleProcess::CConsoleProcess()
{
	m_pCallback=NULL;
	m_hWaitProcess=NULL;
}

CConsoleProcess::~CConsoleProcess()
{
	Close();
}

bool CConsoleProcess::Start(const wchar_t* pszModule, const wchar_t* pszArgs, bool bErrorToOutput, bool bPostCallbacks, IConsoleProcessCallback* pCallback)
{
	ASSERT(pCallback!=NULL);
	ASSERT(m_hProcess==NULL);

	m_pCallback=pCallback;
	m_bPostCallbacks=bPostCallbacks;

	// Open pipes
	m_Pipes.Open(bErrorToOutput);
	
	// Setup startup info
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb=sizeof(si);
	si.dwFlags=STARTF_USESTDHANDLES;
	si.hStdError=m_Pipes.m_hPipeErrorWrite;
	si.hStdInput=m_Pipes.m_hPipeInputRead;
	si.hStdOutput=m_Pipes.m_hPipeOutputWrite;

	// Setup process info
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));

	// Create the process
	CUniString str=pszArgs;
	if (!CreateProcess(pszModule, str.GetBuffer(), NULL, NULL, TRUE, CREATE_SUSPENDED|CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		m_Pipes.Close();
		return false;
	}

	// Get a notification when process finishes
	RegisterWaitForSingleObject(&m_hWaitProcess, pi.hProcess, WaitProcessCallback, this, INFINITE, WT_EXECUTEDEFAULT|WT_EXECUTEONLYONCE);

	// Start readers
	m_OutputReader.Start(m_Pipes.m_hPipeOutputRead, this);
	if (!bErrorToOutput)
		m_ErrorReader.Start(m_Pipes.m_hPipeErrorRead, this);


	// Close child handles now that the process is started so that ReadFile will terminate on the pipe being
	// broken
	m_Pipes.CloseChildHandles();

	// Resume the process now that we're all wired up
	ResumeThread(pi.hThread);

	// Store the process handle, close the thread handle
	m_hProcess=pi.hProcess;
	CloseHandle(pi.hThread);

	return true;
}

void CConsoleProcess::Close()
{
	ATLTRACE(_T("Closing console process...\n"));

	if (m_hWaitProcess)
	{
		UnregisterWait(m_hWaitProcess);
		m_hWaitProcess=NULL;
	}
	m_pCallback=NULL;
	m_hProcess.Release();
	m_OutputReader.Close();
	m_ErrorReader.Close();
	m_Pipes.Close();
}

void CConsoleProcess::Wait()
{
	ASSERT(m_hProcess!=NULL);

	WaitForSingleObject(m_hProcess, INFINITE);
}

bool CConsoleProcess::Write(const char* psz, int cb)
{
	DWORD cbWritten;
	return WriteFile(m_Pipes.m_hPipeInputWrite, psz, cb, &cbWritten, NULL) && cbWritten==cb;
}

bool CConsoleProcess::Write(const char* psz)
{
	return Write(psz, lstrlenA(psz));
}

void CConsoleProcess::OnPipeRead(CPipeReader* pSource, const char* psz, int cb)
{
	if (m_bPostCallbacks)
	{
		COutput* pOutput=new COutput(pSource, psz, cb);

		{
			CEnterCriticalSection ecs(&m_cs);
			m_OutputBuffer.Add(pOutput);
		}

		Post(0);
	}
	else
	{
		m_pCallback->OnOutput(pSource==&m_OutputReader, psz, cb);
	}
}

LRESULT CConsoleProcess::OnMessage(UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	if (nMessage==0)
	{
		COutput* pOutput;
		while (m_OutputBuffer.Dequeue(pOutput))
		{
			m_pCallback->OnOutput(pOutput->m_pSource==&m_OutputReader, pOutput->m_str, pOutput->m_str.GetLength());
			delete pOutput;
		}
	}
	else if (nMessage==1)
	{
		m_pCallback->OnProcessFinished();
	}
	return 0;
}

void CALLBACK CConsoleProcess::WaitProcessCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	CConsoleProcess* pThis=(CConsoleProcess*)lpParameter;
	if (pThis->m_bPostCallbacks)
	{
		pThis->Post(1);
	}
	else
	{
		pThis->m_pCallback->OnProcessFinished();
	}
};




