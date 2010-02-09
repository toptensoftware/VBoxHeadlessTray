//////////////////////////////////////////////////////////////////////////
// ConsoleProcess.h - declaration of ConsoleProcess

#ifndef __CONSOLEPROCESS_H
#define __CONSOLEPROCESS_H

class CProcessPipes
{
public:
			CProcessPipes();
	virtual	~CProcessPipes();

	bool Open(bool bErrorToOutput);
	void Close();
	void CloseParentHandles();
	void CloseChildHandles();

protected:
	bool OpenInternal(bool bErrorToOutput);

public:
	CSmartHandle<HANDLE> m_hPipeInputWrite;
	CSmartHandle<HANDLE> m_hPipeInputRead;
	CSmartHandle<HANDLE> m_hPipeOutputWrite;
	CSmartHandle<HANDLE> m_hPipeOutputRead;
	CSmartHandle<HANDLE> m_hPipeErrorWrite;
	CSmartHandle<HANDLE> m_hPipeErrorRead;
};

class CPipeReader;

class IPipeCallback
{
public:
	virtual void OnPipeRead(CPipeReader* pSource, const char* psz, int cb)=0;
};

class CPipeReader : 
	public CThread
{
public:
			CPipeReader();
	virtual ~CPipeReader();

	bool Start(HANDLE hPipe, IPipeCallback* pCallback);
	void Close();

protected:
	virtual DWORD ThreadProc();

	HANDLE					m_hPipe;
	IPipeCallback*			m_pCallback;
};

class IConsoleProcessCallback
{
public:
	virtual void OnOutput(bool bStdError, const char* psz, int cb)=0;
	virtual void OnProcessFinished()=0;
};

class CConsoleProcess : 
	public IPipeCallback,
	public CPostableObject
{
public:
			CConsoleProcess();
	virtual ~CConsoleProcess();

	bool Start(const wchar_t* pszModule, const wchar_t* pszArgs, bool bErrorToOutput, bool bPostCallbacks, IConsoleProcessCallback* pCallback);
	void Close();
	void Wait();
	bool Write(const char* psz, int cb);
	bool Write(const char* psz);

protected:
	virtual void OnPipeRead(CPipeReader* pSource, const char* psz, int cb);
	virtual LRESULT OnMessage(UINT nMessage, WPARAM wParam, LPARAM lParam);
	static void CALLBACK WaitProcessCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired);

	class COutput
	{
	public:
		COutput(CPipeReader* pSource, const char* psz, int cb) : 
			m_pSource(pSource),
			m_str(psz, cb)
		{
		}
		CPipeReader*	m_pSource;
		CAnsiString			m_str;
	};

	CVector<COutput*, SOwnedPtr>	m_OutputBuffer;
	CProcessPipes					m_Pipes;
	CSmartHandle<HANDLE>			m_hProcess;
	CPipeReader						m_OutputReader;
	CPipeReader						m_ErrorReader;
	CCriticalSection				m_cs;
	bool							m_bPostCallbacks;
	IConsoleProcessCallback*		m_pCallback;
	HANDLE							m_hWaitProcess;
};




#endif	// __CONSOLEPROCESS_H

