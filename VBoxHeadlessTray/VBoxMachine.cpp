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
	m_dwHeadlessPid=0;
	m_hHeadlessProcess=NULL;
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

	// Connect listsner for IVirtualBoxCallback
	log("Registering virtualbox callback\n");
	m_spVirtualBox->RegisterCallback(&m_VirtualBoxEvents);
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
		m_spVirtualBox->UnregisterCallback(&m_VirtualBoxEvents);
		m_bCallbackRegistered=false;
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

void CVBoxMachine::OnMachineStateChange(const wchar_t* pszMachineID, MachineState State)
{
	if (!IsEqualString(pszMachineID, m_bstrMachineID))
		return;

	m_State=State;

	switch (m_State)
	{
		case MachineState_PoweredOff:
		case MachineState_Saved:
		case MachineState_Teleported:
		case MachineState_Aborted:
			m_dwHeadlessPid=0;
			CloseHandle(m_hHeadlessProcess);
			m_hHeadlessProcess=NULL;
			break;
	}


	if (m_pEvents)
	{
		m_pEvents->OnStateChange(State);
	}
}


HRESULT CVBoxMachine::Exec(const wchar_t* pszCommandLine, DWORD* ppid, HANDLE* phProcess)
{
	// Work out path to vbox
	CUniString strVBoxPath;
	GetSpecialFolderLocation(CSIDL_PROGRAM_FILES, L"Sun\\VirtualBox", false, strVBoxPath);

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
	if (!CreateProcess(NULL, str.GetBuffer(), NULL, NULL, TRUE, 0/*CREATE_NO_WINDOW*/, NULL, NULL, &si, &pi))
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

	ASSERT(m_hHeadlessProcess==NULL);
	Exec(L"\"{vboxdir}\\VBoxHeadless.exe\" --startvm \"{machinename}\"", &m_dwHeadlessPid, &m_hHeadlessProcess);
	return true;
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
		if (SUCCEEDED(m_spVirtualBox->OpenExistingSession(spSession, m_bstrMachineID)))
		{
			CComPtr<IConsole> spConsole;
			if (SUCCEEDED(spSession->get_Console(&spConsole)))
			{
				CComPtr<IProgress> spProgress;
				spConsole->SaveState(&spProgress);
			}
		}

	}

	return true;
}

bool CVBoxMachine::PowerDown()
{
	Exec(L"\"{vboxdir}\\VBoxManage.exe\" controlvm \"{machinename}\" poweroff");
	return true;
}

bool CVBoxMachine::Pause()
{
	Exec(L"\"{vboxdir}\\VBoxManage.exe\" controlvm \"{machinename}\" pause");
	return true;
}

bool CVBoxMachine::Resume()
{
	Exec(L"\"{vboxdir}\\VBoxManage.exe\" controlvm \"{machinename}\" resume");
	return true;
}

bool CVBoxMachine::Reset()
{
	Exec(L"\"{vboxdir}\\VBoxManage.exe\" controlvm \"{machinename}\" reset");
	return true;
}

