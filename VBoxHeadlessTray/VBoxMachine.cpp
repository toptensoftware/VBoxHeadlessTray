//////////////////////////////////////////////////////////////////////////
// VBoxMachine.cpp - implementation of CVBoxMachine class

#include "stdafx.h"
#include "VBoxHeadlessTray.h"

#include "VBoxMachine.h"

/*
void WinExec(const wchar_t* pszCommandLine, int nShow)
{
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb=sizeof(si);
	si.wShowWindow=nShow;
	PROCESS_INFORMATION pi;
	if (CreateProcess(NULL, CUniString(pszCommandLine).GetBuffer(0), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}
*/

const wchar_t* GetMachineStateDescription(MachineState state)
{
	const wchar_t* pszStates[]= {
		L"Powered Off", 
		L"Powered Off",
		L"Saved",
		L"Aborted",
		L"Running",
		L"Paused",
		L"Stuck",
		L"Starting",
		L"Stopping",
		L"Saving",
		L"Restoring",
		L"Discarding",
		L"Setting Up",
	};
		
	const wchar_t* pszState=L"Unknown State";
	if (state>=0 && state<_countof(pszStates))
	{
		pszState=pszStates[state];
	}

	return pszState;
}

void ClearComErrorInfo()
{
	SetErrorInfo(0, NULL);
}

CUniString vboxFormatError(HRESULT hr)
{
	// Get error info
	Simple::CAutoPtr<IErrorInfo, SRefCounted> spErrorInfo;
	GetErrorInfo(0, &spErrorInfo);

	CUniString str;
	if (spErrorInfo)
	{
		BSTR bstr=NULL;
		spErrorInfo->GetDescription(&bstr);
		str=bstr;
		SysFreeString(bstr);
	}
	else
	{
		str=FormatError(hr);
	}

	return str;
}


//////////////////////////////////////////////////////////////////////////
// CFramebuffer

// Constructor
CFramebuffer::CFramebuffer()
{
	m_pVRAM=NULL;
	m_nWidth=1024;
	m_nHeight=768;
	m_nBPP=32;
	m_nBytesPerLine=m_nWidth * 4;
	m_nPixelFormat=FramebufferPixelFormat_FOURCC_RGB;
	m_bUsesGuestVRAM=false;

	//BOOL bTemp=FALSE;
	//RequestResize(0, m_nPixelFormat, NULL, m_nBPP, m_nBytesPerLine, m_nWidth, m_nHeight, &bTemp);
}

// Destructor
CFramebuffer::~CFramebuffer()
{
	FreeVRAM();
}

// Free any allocated video ram
void CFramebuffer::FreeVRAM()
{
	// Quit if not allocated
	if (!m_pVRAM)
		return;

	// Dont free guest VRAM
	if (!m_bUsesGuestVRAM)
	{
		free(m_pVRAM);
	}

	// Zap ptr
	m_pVRAM=NULL;
}



// Implementation of get_Address
STDMETHODIMP CFramebuffer::get_Address(BYTE * * aAddress)
{
	*aAddress=m_pVRAM;
	return S_OK;
}

// Implementation of get_Width
STDMETHODIMP CFramebuffer::get_Width(ULONG * aWidth)
{
	*aWidth=m_nWidth;
	return S_OK;
}

// Implementation of get_Height
STDMETHODIMP CFramebuffer::get_Height(ULONG * aHeight)
{
	*aHeight=m_nHeight;
	return S_OK;
}

// Implementation of get_BitsPerPixel
STDMETHODIMP CFramebuffer::get_BitsPerPixel(ULONG * aBitsPerPixel)
{
	*aBitsPerPixel=m_nBPP;
	return S_OK;
}

// Implementation of get_BytesPerLine
STDMETHODIMP CFramebuffer::get_BytesPerLine(ULONG * aBytesPerLine)
{
	*aBytesPerLine=m_nBytesPerLine;
	return S_OK;
}

// Implementation of get_PixelFormat
STDMETHODIMP CFramebuffer::get_PixelFormat(ULONG * aPixelFormat)
{
	*aPixelFormat=m_nPixelFormat;
	return S_OK;
}

// Implementation of get_UsesGuestVRAM
STDMETHODIMP CFramebuffer::get_UsesGuestVRAM(BOOL * aUsesGuestVRAM)
{
	*aUsesGuestVRAM=m_bUsesGuestVRAM;
	return S_OK;
}

// Implementation of get_HeightReduction
STDMETHODIMP CFramebuffer::get_HeightReduction(ULONG * aHeightReduction)
{
	*aHeightReduction=0;
	return S_OK;
}

// Implementation of get_Overlay
STDMETHODIMP CFramebuffer::get_Overlay(IFramebufferOverlay * * aOverlay)
{
	*aOverlay=NULL;
	return S_OK;
}

// Implementation of get_WinId
STDMETHODIMP CFramebuffer::get_WinId(ULONG64 * aWinId)
{
	aWinId=0;
	return S_OK;
}

// Implementation of Lock
STDMETHODIMP CFramebuffer::Lock()
{
	EnterCriticalSection(&m_cs);
	return S_OK;
}

// Implementation of Unlock
STDMETHODIMP CFramebuffer::Unlock()
{
	LeaveCriticalSection(&m_cs);
	return S_OK;
}

// Implementation of NotifyUpdate
STDMETHODIMP CFramebuffer::NotifyUpdate(
        ULONG aX,
        ULONG aY,
        ULONG aWidth,
        ULONG aHeight
    )
{
	// Nothing to do!
	return S_OK;
}

// Implementation of RequestResize
STDMETHODIMP CFramebuffer::RequestResize(
        ULONG aScreenId,
        ULONG aPixelFormat,
        BYTE * aVRAM,
        ULONG aBitsPerPixel,
        ULONG aBytesPerLine,
        ULONG aWidth,
        ULONG aHeight,
        BOOL * aFinished
    )
{
	// Free old video memory
	FreeVRAM();

	// Store passed in params
	m_pVRAM=aVRAM;
	m_nBPP=aBitsPerPixel;
	m_nBytesPerLine=aBytesPerLine;
	m_nWidth=aWidth;
	m_nHeight=aHeight;
	m_nPixelFormat=aPixelFormat;

	// Work out whether to use guest VRAM or allocate our own
    m_bUsesGuestVRAM = false;
    if (m_nPixelFormat == FramebufferPixelFormat_FOURCC_RGB && m_pVRAM!=NULL)
    {
        switch (m_nBPP)
        {
            case 16:
            case 24:
            case 32:
                m_bUsesGuestVRAM = true;
                break;
		}
	}

	if (!m_bUsesGuestVRAM)
	{
		// If not using guest VRAM, always use 32bpp format
        m_nPixelFormat=FramebufferPixelFormat_FOURCC_RGB;
        m_nBPP=32;
        m_nBytesPerLine = m_nWidth * 4;

		// Allocate memory buffer
		m_pVRAM=(BYTE*)malloc(m_nHeight * m_nBytesPerLine);
	}


	*aFinished=TRUE;

	return S_OK;
}

// Implementation of VideoModeSupported
STDMETHODIMP CFramebuffer::VideoModeSupported(
        ULONG aWidth,
        ULONG aHeight,
        ULONG aBpp,
        BOOL * aSupported
    )
{
	// Any 32-bpp format is ok
	*aSupported=aBpp==32;
	return S_OK;
}

// Implementation of GetVisibleRegion
STDMETHODIMP CFramebuffer::GetVisibleRegion(
        BYTE * aRectangles,
        ULONG aCount,
        ULONG * aCountCopied
    )
{
	return E_NOTIMPL;
}

// Implementation of SetVisibleRegion
STDMETHODIMP CFramebuffer::SetVisibleRegion(
        BYTE * aRectangles,
        ULONG aCount
    )
{
	return E_NOTIMPL;
}

// Implementation of ProcessVHWACommand
STDMETHODIMP CFramebuffer::ProcessVHWACommand(
        BYTE * aCommand
	)
{
	// Not supported
	return E_NOTIMPL;
}



//////////////////////////////////////////////////////////////////////////
// CVBoxMachine

// Constructor
CVBoxMachine::CVBoxMachine()
{
	m_pEvents=NULL;
	m_State=MachineState_Null;
}

// Destructor
CVBoxMachine::~CVBoxMachine()
{
}

void CVBoxMachine::SetMachineName(const wchar_t* psz)
{
	ASSERT(!m_spMachine);
	m_strMachineName=psz;
}

CUniString CVBoxMachine::GetMachineName()
{
	return m_strMachineName;
}

void CVBoxMachine::SetEventHandler(IVBoxMachineEvents* pEvents)
{
	m_pEvents=pEvents;
}

MachineState CVBoxMachine::GetState()
{
	return m_State;
}

CUniString CVBoxMachine::GetErrorMessage()
{
	return m_strLastError;
}

void CVBoxMachine::Close()
{
	// Remove callback
	if (m_spConsole)
	{
		log("Unregistering console callback\n");
		m_spConsole->UnregisterCallback(&m_ConsoleCallback);
	}

	// Close session
	if (m_spSession)
	{
		log("Closing session\n");
		m_spSession->Close();
	}

	// Release objects
	log("Releasing console\n");
	m_spConsole.Release();

	log("Releasing session\n");
	m_spSession.Release();

	log("Releasing VirtualBox\n");
	m_spVirtualBox.Release();

	m_spMachine.Release();
	m_bstrMachineID.Empty();

	m_State=MachineState_Null;

	log_close();
}

// Set error message
bool CVBoxMachine::SetError(const wchar_t* psz)
{
	log("Error: %S\n", psz);

	m_strLastError=psz;
	if (m_pEvents)
	{
		m_pEvents->OnError(psz);
	}
	return false;
}

#ifndef _F_INPROC_VM
HRESULT CVBoxMachine::StartHeadlessProcess()
{
	// Work out path to vbox
	CUniString strVBoxPath;
	GetSpecialFolderLocation(CSIDL_PROGRAM_FILES, L"Sun\\VirtualBox", false, strVBoxPath);

	// Get path to headless
	CUniString strHeadlessPath=SimplePathAppend(strVBoxPath, L"VBoxHeadless.exe");

	// Work out log file path
	CComBSTR bstrSettingsPath;
	m_spMachine->get_SettingsFilePath(&bstrSettingsPath);
	CUniString strLogFile=ChangeFileName(bstrSettingsPath, Format(L"%s-VBoxHeadless.log", ExtractFileTitle(bstrSettingsPath)));

	// Create log file
	SECURITY_ATTRIBUTES sa;
	sa.nLength= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	CSmartHandle<HANDLE> hLogFileOutput=CreateFile(strLogFile, GENERIC_WRITE|GENERIC_READ, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hLogFileOutput==INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());

	// Duplicate log file handle for error output
	CSmartHandle<HANDLE> hLogFileError;
	if (!DuplicateHandle(GetCurrentProcess(), hLogFileOutput, GetCurrentProcess(), &hLogFileError, 0, TRUE,DUPLICATE_SAME_ACCESS))
		return HRESULT_FROM_WIN32(GetLastError());

	// Setup startup info
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb=sizeof(si);
	si.dwFlags=STARTF_USESTDHANDLES;
	si.hStdError=hLogFileError;
	si.hStdInput=INVALID_HANDLE_VALUE;
	si.hStdOutput=hLogFileOutput;

	// Setup process info
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));

	// Create the process
	CUniString str=Format(L"\"%s\" --startvm \"%s\"", strHeadlessPath, m_strMachineName);
	if (!CreateProcess(NULL, str.GetBuffer(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// Close handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return S_OK;
}
#endif

bool CVBoxMachine::PowerUp()
{
	log("Powering up %S\n", m_strMachineName);

	m_pEvents->OnStateChange(MachineState_Starting);

	m_strLastError.Empty();

	// Create VirtualBox object
	log("Creating VirtualBox\n");
	ClearComErrorInfo();
	HRESULT hr=m_spVirtualBox.CoCreateInstance(__uuidof(VirtualBox));
	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to create VirtualBox COM server - %s", vboxFormatError(hr)));
	}

	// Get machine ID
	log("Finding machine\n");
	ClearComErrorInfo();
	CComPtr<IMachine> spMachine;
	hr=m_spVirtualBox->FindMachine(CComBSTR(m_strMachineName), &spMachine);
	if (FAILED(hr) || spMachine==NULL)
	{
		return SetError(Format(L"Machine \"%s\" not found - %s", m_strMachineName, vboxFormatError(hr)));
	}
	spMachine->get_Id(&m_bstrMachineID);

	// Open log file
	CComBSTR bstrLogFolder;
	spMachine->get_LogFolder(&bstrLogFolder);
	log_open(w2a(SimplePathAppend(bstrLogFolder, L"VBoxHeadlessTray.log")));

	spMachine.Release();

	

	// Create Session Object
	log("Creating session\n");
	ClearComErrorInfo();
	hr=m_spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to create VirtualBox session object - %s", vboxFormatError(hr)));
	}

	// Open session
	log("Opening session\n");
	ClearComErrorInfo();
#ifdef _F_INPROC_VM
	hr=m_spVirtualBox->OpenSession(m_spSession, m_bstrMachineID);
	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to open session for machine \"%s\" - %s", m_strMachineName, vboxFormatError(hr)));
	}
#else


	hr=StartHeadlessProcess();
	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to launch headless process for \"%s\" - %s", m_strMachineName, FormatError(hr)));
	}

	for (int i=0; i<20; i++)
	{
		Sleep(100);
		hr=m_spVirtualBox->OpenExistingSession(m_spSession, m_bstrMachineID);
		if (hr!=VBOX_E_INVALID_SESSION_STATE)
			break;
	}

	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to open existing session for \"%s\" - %s", m_strMachineName, vboxFormatError(hr)));
	}

#endif

	// Get the console
	log("Getting console\n");
	ClearComErrorInfo();
	hr=m_spSession->get_Console(&m_spConsole);
	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to get console for machine \"%s\" - %s", m_strMachineName, vboxFormatError(hr)));
	}

	// Register for callback events from the console
	log("Registering console callback\n");
	ClearComErrorInfo();
	hr=m_spConsole->RegisterCallback(&m_ConsoleCallback);
	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to register callbacks for machine \"%s\" - %s", m_strMachineName, vboxFormatError(hr)));
	}

	// Get display
	log("Getting display\n");
	CComPtr<IDisplay> spDisplay;
	hr=m_spConsole->get_Display(&spDisplay);
	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to get display object for \"%s\" - %s", m_strMachineName, vboxFormatError(hr)));
	}

	// Get the machine
	m_spConsole->get_Machine(&m_spMachine);

	/*
	// Check if VRDP server is enabled
	BOOL bVrdpEnabled=FALSE;
	CComPtr<IVRDPServer> spVRDPServer;
	m_spMachine->get_VRDPServer(&spVRDPServer);
	if (spVRDPServer)
	{
		spVRDPServer->get_Enabled(&bVrdpEnabled);
	}

	// Only create frame buffers is VRDP is enabled
	if (bVrdpEnabled && FALSE)
	{
		// Create frame buffers
		ULONG nMonitors = 1;
		m_spMachine->get_MonitorCount(&nMonitors);
		for (ULONG i=0; i<nMonitors; i++)
		{
			// Create an instance of Framebuffer object
			CComObject<CFramebuffer>* pFramebuffer;
			CComObject<CFramebuffer>::CreateInstance(&pFramebuffer);
			CComPtr<IFramebuffer> spFramebuffer(pFramebuffer);

			// Set it
			spDisplay->SetFramebuffer(i, spFramebuffer);
		}
	}
	*/

	// Power up the console
	log("Powering up...\n");

#ifdef _F_INPROC_VM
	ClearComErrorInfo();
	CComPtr<IProgress> spProgress;
	return HandleProgress(L"power up", m_spConsole->PowerUp(&spProgress), spProgress);
#else
	return true;
#endif
}

bool CVBoxMachine::HandleResult(const wchar_t* pszOp, HRESULT hr)
{
	if (hr==S_OK)
		return true;

	return SetError(Format(L"Failed to %s \"%s\" - %s", pszOp, m_strMachineName, vboxFormatError(hr)));
}


bool CVBoxMachine::HandleProgress(const wchar_t* pszOp, HRESULT hr, IProgress* pProgress)
{
	m_strLastError.Empty();

	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to %s \"%s\" - %s", pszOp, m_strMachineName, vboxFormatError(hr)));
	}

	log("Waiting for completion...\n");

	if (pProgress && SUCCEEDED(pProgress->WaitForCompletion(-1)))
	{
		LONG res;
		pProgress->get_ResultCode(&res);
		if (FAILED(res))
		{
			CComBSTR bstrError;
			CComPtr<IVirtualBoxErrorInfo> spErrorInfo;
			pProgress->get_ErrorInfo(&spErrorInfo);
			if (spErrorInfo)
			{
				spErrorInfo->GetDescription(&bstrError);
			}
			else
			{
				bstrError=L"No error returned by VirtualBox";
			}
			return SetError(Format(L"Failed to %s \"%s\" - %s - %s", pszOp, m_strMachineName, bstrError, vboxFormatError(res)));
		}
	}

	log("OK\n");

	return true;
}

bool CVBoxMachine::SaveState()
{
	if (!m_spConsole)
		return true;

	m_strLastError.Empty();

	log("Saving state\n");
	CComPtr<IProgress> spProgress;
	return HandleProgress(L"save state of ", m_spConsole->SaveState(&spProgress), spProgress);
}

bool CVBoxMachine::PowerDown()
{
	if (!m_spConsole)
		return true;

	log("Powering down\n");
	CComPtr<IProgress> spProgress;
	return HandleProgress(L"power off", m_spConsole->PowerDown(&spProgress), spProgress);
}

bool CVBoxMachine::Pause()
{
	if (!m_spConsole)
		return true;

	log("Pausing\n");
	return HandleResult(L"pause", m_spConsole->Pause());
}

bool CVBoxMachine::Resume()
{
	if (!m_spConsole)
		return true;

	log("Resuming\n");
	return HandleResult(L"resume", m_spConsole->Resume());
}

bool CVBoxMachine::Reset()
{
	if (!m_spConsole)
		return true;

	log("Resetting\n");
	return HandleResult(L"reset", m_spConsole->Reset());
}

LRESULT CVBoxMachine::OnMessage(UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	switch (nMessage)
	{
		case VMM_MACHINE_STOPPED:
			log("Machine stopped, closing.\n");
			m_pEvents->OnStateChange(MachineState_Null);
			Close();
			break;

	}

	return 0;
}

