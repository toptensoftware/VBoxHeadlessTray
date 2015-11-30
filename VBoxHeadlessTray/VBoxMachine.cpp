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
		L"Null",
		L"Powered Off",
		L"Saved",
		L"Teleported",
		L"Aborted",
		L"Running",
		L"Paused",
		L"Stuck",
		L"Teleporting",
		L"Live Snapshotting",
		L"Starting",
		L"Stopping",
		L"Saving",
		L"Restoring",
		L"Teleporting Paused VM",
		L"Teleporting In",
		L"Fault Tolerant Syncing",
		L"Deleting Snapshot Online",
		L"Deleting Snapshot Paused",
		L"Restoring Snapshot",
		L"Deleting Snapshot",
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
// CVBoxMachine

// Constructor
CVBoxMachine::CVBoxMachine()
{
	m_pEvents=NULL;
	m_State=MachineState_Null;
	m_bCallbackRegistered=false;
	m_hPollTimer = NULL;
}

// Destructor
CVBoxMachine::~CVBoxMachine()
{
}

void CVBoxMachine::SetMachineName(const wchar_t* psz)
{
	ASSERT(!IsOpen());
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

HRESULT CVBoxMachine::Open()
{
	// Check not already open
	ASSERT(!IsOpen());

	// Create VirtualBox object
	log("Creating VirtualBox\n");
	ClearComErrorInfo();
	HRESULT hr=m_spVirtualBox.CoCreateInstance(__uuidof(VirtualBox));
	if (FAILED(hr))
	{
		Close();
		return SetError(Format(L"Failed to create VirtualBox COM server - %s", vboxFormatError(hr)));
	}

	// Get machine ID
	log("Finding machine\n");
	ClearComErrorInfo();
	hr=m_spVirtualBox->FindMachine(CComBSTR(m_strMachineName), &m_spMachine);
	if (FAILED(hr) || m_spMachine==NULL)
	{
		Close();
		return SetError(Format(L"Machine \"%s\" not found - %s", m_strMachineName, vboxFormatError(hr)));
	}
	m_spMachine->get_Id(&m_bstrMachineID);

	// Open log file
	CComBSTR bstrLogFolder;
	m_spMachine->get_LogFolder(&bstrLogFolder);
	log_open(w2a(SimplePathAppend(bstrLogFolder, L"VBoxHeadlessTray.log")));

	// Connect listener for IVirtualBoxCallback
	log("Registering virtualbox callback\n");
	CComPtr<IEventSource> spEventSource;
	m_spVirtualBox->get_EventSource(&spEventSource);


	SAFEARRAYBOUND bound;
	bound.lLbound=0;
	bound.cElements=1;
	SAFEARRAY* interesting = SafeArrayCreate(VT_I4, 1, &bound);
	int* pinteresting;
	SafeArrayAccessData(interesting, (void**)&pinteresting);
	pinteresting[0]=VBoxEventType_OnMachineStateChanged;
	SafeArrayUnaccessData(interesting);
	spEventSource->RegisterListener(&m_EventListener, interesting, VARIANT_TRUE);
	SafeArrayDestroy(interesting);

	// Since VBox 4.1 seems to fail to send machine state events, lets just poll the machine every 5 seconds
	// If we get a real machine state event, we'll kill this, assuming it's working
	// The VBoxHeadlessTray machine window also calls OnMachineStateChanged from it's tray icon
	// update timer.... so when things are transitioning the state should update fairly quickly.
	m_hPollTimer = SetCallbackTimer(1000, 0, OnPollMachineState, (LPARAM)this);

	m_bCallbackRegistered=true;

	m_spMachine->get_State(&m_State);


	return S_OK;
}

void CVBoxMachine::Close()
{
	if (m_bCallbackRegistered)
	{
		// Connect listsner for IVirtualBoxCallback
		log("Unregistering virtualbox callback\n");
		CComPtr<IEventSource> spEventSource;
		m_spVirtualBox->get_EventSource(&spEventSource);
		spEventSource->UnregisterListener(&m_EventListener);
		m_bCallbackRegistered=false;
	}

	if (m_hPollTimer!=NULL)
	{
		KillCallbackTimer(m_hPollTimer);
		m_hPollTimer=NULL;
	}

	log("Releasing Machine\n");
	m_spMachine.Release();

	log("Releasing VirtualBox\n");
	m_spVirtualBox.Release();

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

void CVBoxMachine::OnMachineStateChange(IMachineStateChangedEvent* e)
{
	if (e!=NULL)
	{
		// A real machine state change event!

		CComBSTR bstrMachineID;
		e->get_MachineId(&bstrMachineID);
		if (!IsEqualStringI(bstrMachineID, m_bstrMachineID))
			return;

		// Get new state
		e->get_State(&m_State);

		// Since we got an actual event from VirtualBox, let's assume it's working
		// and kill our poll timer
		if (m_hPollTimer!=NULL)
		{
			KillCallbackTimer(m_hPollTimer);
		}
	}
	else
	{
		// Poll mode

		// Get the current state and see if it changed
		MachineState newState;
		m_spMachine->get_State(&newState);
		if (newState == m_State)
			return;

		// Yep
		m_State = newState;
	}

	switch (m_State)
	{
		case MachineState_PoweredOff:
		case MachineState_Saved:
		case MachineState_Teleported:
		case MachineState_Aborted:
			break;
	}


	if (m_pEvents)
	{
		m_pEvents->OnStateChange(m_State);
	}
}


HRESULT CVBoxMachine::Exec(const wchar_t* pszCommandLine, DWORD* ppid, HANDLE* phProcess)
{
	// Get from the InstallDir registry key
	CUniString strVBoxPath;
	RegGetString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Oracle\\VirtualBox", L"InstallDir", strVBoxPath);

	// If that fails, assume program files...
	if (strVBoxPath.IsEmpty())
	{
		// Work out path to vbox
		GetSpecialFolderLocation(CSIDL_PROGRAM_FILES, L"Oracle\\VirtualBox", false, strVBoxPath);
	}

	// Remove trailing backslash
	RemoveTrailingBackslash(strVBoxPath.GetBuffer());

	CUniString str=StringReplace(pszCommandLine, L"{vboxdir}", strVBoxPath, true);
	str=StringReplace(str, L"{machinename}", m_strMachineName, true);

	// Setup startup info
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb=sizeof(si);

	// Setup process info
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));

	// Create the process
	if (!CreateProcess(NULL, str.GetBuffer(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}


	// Close/return handles
	if (phProcess)
	{
		phProcess[0]=pi.hProcess;
	}
	else
	{
		CloseHandle(pi.hProcess);
	}
	CloseHandle(pi.hThread);

	// Return the process ID
	if (ppid)
	{
		ppid[0]=pi.dwProcessId;
	}

	return S_OK;
}

bool CVBoxMachine::PowerUp()
{	
	if (m_State>=MachineState_Running)
		return true;

	CComPtr<ISession> spSession;
	HRESULT hr = spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to create VirtualBox session object - %s", vboxFormatError(hr)));
	}

	CComPtr<IProgress> spProgress;
	return SUCCEEDED(m_spMachine->LaunchVMProcess(spSession, CComBSTR("headless"), CComBSTR(""), &spProgress));
}

bool CVBoxMachine::OpenGUI()
{
	if (m_State >= MachineState_Running)
		return true;

	CComPtr<ISession> spSession;
	HRESULT hr = spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
	{
		return SetError(Format(L"Failed to create VirtualBox session object - %s", vboxFormatError(hr)));
	}

	CComPtr<IProgress> spProgress;
	return SUCCEEDED(m_spMachine->LaunchVMProcess(spSession, CComBSTR("gui"), CComBSTR(""), &spProgress));
}


bool CVBoxMachine::SaveState()
{
	// We need to do save state without creating VBoxManage process since, we can't launch a process
	// while Windows is shutting down.
	if (m_spMachine)
	{
		log("Creating session\n");
		ClearComErrorInfo();
		CComPtr<ISession> spSession;
		HRESULT hr=spSession.CoCreateInstance(__uuidof(Session));
		if (FAILED(hr))
		{
			return SetError(Format(L"Failed to create VirtualBox session object - %s", vboxFormatError(hr)));
		}

		// Open existing session
		if (SUCCEEDED(m_spMachine->LockMachine(spSession, LockType_Shared)))
		//if (SUCCEEDED(m_spVirtualBox->OpenExistingSession(spSession, m_bstrMachineID)))
		{
			CComPtr<IMachine> spMachine;
			if (SUCCEEDED(spSession->get_Machine(&spMachine)))
			{
				CComPtr<IProgress> spProgress;
				spMachine->SaveState(&spProgress);
			}

			spSession->UnlockMachine();
		}

	}

	return true;
}

bool CVBoxMachine::PowerDown()
{
	if (!m_spMachine)
		return false;

	CComPtr<ISession> spSession;
	HRESULT hr = spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
		return false;

	// Open existing session
	if (FAILED(m_spMachine->LockMachine(spSession, LockType_Shared)))
		//if (FAILED(m_spVirtualBox->OpenExistingSession(spSession, m_bstrMachineID)))
		return false;

	CComPtr<IConsole> spConsole;
	if (FAILED(spSession->get_Console(&spConsole)) || !spConsole)
	{
		spSession->UnlockMachine();
		return false;
	}

	CComPtr<IProgress> spProgress;
	hr=spConsole->PowerDown(&spProgress);
	spSession->UnlockMachine();
	return SUCCEEDED(hr);
}

bool CVBoxMachine::Pause()
{
	if (!m_spMachine)
		return false;

	CComPtr<ISession> spSession;
	HRESULT hr = spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
		return false;

	// Open existing session
	if (FAILED(m_spMachine->LockMachine(spSession, LockType_Shared)))
		//if (FAILED(m_spVirtualBox->OpenExistingSession(spSession, m_bstrMachineID)))
		return false;

	CComPtr<IConsole> spConsole;
	if (FAILED(spSession->get_Console(&spConsole)) || !spConsole)
	{
		spSession->UnlockMachine();
		return false;
	}

	hr=spConsole->Pause();
	spSession->UnlockMachine();
	return SUCCEEDED(hr);
}

bool CVBoxMachine::Resume()
{
	if (!m_spMachine)
		return false;

	CComPtr<ISession> spSession;
	HRESULT hr = spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
		return false;

	// Open existing session
	if (FAILED(m_spMachine->LockMachine(spSession, LockType_Shared)))
		//if (FAILED(m_spVirtualBox->OpenExistingSession(spSession, m_bstrMachineID)))
		return false;

	CComPtr<IConsole> spConsole;
	if (FAILED(spSession->get_Console(&spConsole)) || !spConsole)
	{
		spSession->UnlockMachine();
		return false;
	}

	hr=spConsole->Resume();
	spSession->UnlockMachine();
	return SUCCEEDED(hr);
}

bool CVBoxMachine::Reset()
{
	if (!m_spMachine)
		return false;

	CComPtr<ISession> spSession;
	HRESULT hr = spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
		return false;

	// Open existing session
	if (FAILED(m_spMachine->LockMachine(spSession, LockType_Shared)))
		//if (FAILED(m_spVirtualBox->OpenExistingSession(spSession, m_bstrMachineID)))
		return false;

	CComPtr<IConsole> spConsole;
	if (FAILED(spSession->get_Console(&spConsole)) || !spConsole)
	{
		spSession->UnlockMachine();
		return false;
	}

	hr=spConsole->Reset();
	spSession->UnlockMachine();
	return SUCCEEDED(hr);
}

bool CVBoxMachine::AcpiPowerButton()
{
	if (!m_spMachine)
		return false;

	CComPtr<ISession> spSession;
	HRESULT hr = spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
		return false;

	// Open existing session
	if (FAILED(m_spMachine->LockMachine(spSession, LockType_Shared)))
		//if (FAILED(m_spVirtualBox->OpenExistingSession(spSession, m_bstrMachineID)))
		return false;

	CComPtr<IConsole> spConsole;
	if (FAILED(spSession->get_Console(&spConsole)) || !spConsole)
	{
		spSession->UnlockMachine();
		return false;
	}

	hr=spConsole->PowerButton();
	spSession->UnlockMachine();
	return SUCCEEDED(hr);
}

bool CVBoxMachine::AcpiSleep()
{
	if (!m_spMachine)
		return false;

	CComPtr<ISession> spSession;
	HRESULT hr = spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
		return false;

	// Open existing session
	if (FAILED(m_spMachine->LockMachine(spSession, LockType_Shared)))
		//if (FAILED(m_spVirtualBox->OpenExistingSession(spSession, m_bstrMachineID)))
		return false;

	CComPtr<IConsole> spConsole;
	if (FAILED(spSession->get_Console(&spConsole)) || !spConsole)
	{
		spSession->UnlockMachine();
		return false;
	}

	hr = spConsole->SleepButton();
	spSession->UnlockMachine();
	return SUCCEEDED(hr);
}

// Check if machine has additions installed
bool CVBoxMachine::AdditionsActive()
{
	if (!m_spMachine)
		return false;

	CComPtr<ISession> spSession;
	HRESULT hr=spSession.CoCreateInstance(__uuidof(Session));
	if (FAILED(hr))
		return false;

	// Open existing session
	if (FAILED(m_spMachine->LockMachine(spSession, LockType_Shared)))
	//if (FAILED(m_spVirtualBox->OpenExistingSession(spSession, m_bstrMachineID)))
		return false;

	CComPtr<IConsole> spConsole;
	if (FAILED(spSession->get_Console(&spConsole)) || !spConsole)
	{
		spSession->UnlockMachine();
		return false;
	}

	CComPtr<IGuest> spGuest;
	if (FAILED(spConsole->get_Guest(&spGuest)) || !spGuest)
	{
		spSession->UnlockMachine();
		return false;
	}

	CComBSTR bstrAdditionsVersion;
	spGuest->get_AdditionsVersion(&bstrAdditionsVersion);

	spSession->UnlockMachine();

	return !IsEmptyString(bstrAdditionsVersion);
}

DWORD CVBoxMachine::GetHeadlessPid()
{
	if (!m_spMachine || m_State!=MachineState_Running)
		return 0;

	// Is it headless?
	CComBSTR bstrName;
	m_spMachine->get_SessionName(&bstrName);
	if (!IsEqualStringI(bstrName, L"headless"))
		return 0;

	unsigned long ulSessionPID;

	m_spMachine->get_SessionPID(&ulSessionPID);


	return ulSessionPID;
}
