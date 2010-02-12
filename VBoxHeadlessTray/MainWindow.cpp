//////////////////////////////////////////////////////////////////////////
// MainWindow.cpp - implementation of MainWindow

#include "stdafx.h"
#include "VBoxHeadlessTray.h"

#include "MainWindow.h"
#include "Utils.h"


CMainWindow::CMainWindow()
{
	m_iSaveStateReason=0;
	m_bTimerRunning=false;
	m_iAnimationFrame=0;
}

CMainWindow::~CMainWindow() 
{
		
};

bool CMainWindow::Create()
{
	return !!CWindowImpl::Create(GetDesktopWindow(), 0, _T("VBoxHeadlessTray"), WS_CHILD);
}

void CMainWindow::Destroy()
{
	DestroyWindow();
}

// WM_CREATE Handler
LRESULT CMainWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// Create tray icon
	m_hIcon=LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICON));
	m_hIconRunning=LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICONRUNNING));
	m_hIconOff=LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICONOFF));
	m_hIconPaused=LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICONPAUSED));
	m_hIconSaved=LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICONSAVED));
	m_hIconAborted=LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICONABORTED));
	m_hIconTransition[0]=LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICONTRANS1));
	m_hIconTransition[1]=LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICONTRANS2));
	m_hIconTransition[2]=LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICONTRANS3));

	m_NotifyIcon.Create(m_hWnd);

	m_Machine.SetMachineName(g_strMachineName);
	m_Machine.SetEventHandler(this);
	HRESULT hr=m_Machine.Open();
	if (FAILED(hr))
	{
		SlxMessageBox(Format(L"Failed to open machine %s - %s", g_strMachineName, m_Machine.GetErrorMessage()));
	}

	// Turn off auto run
	log("Adding autorun entries\n");
	ManageAutoRun(Format(L"VBoxHeadlessTray %s", g_strMachineName), maroSet, Format(L"%s\"%s\"", (g_bPowerOnMachine ? L"" : L"-np "), g_strMachineName));

	UpdateTrayIcon();

	if (g_bPowerOnMachine && m_Machine.GetState()<=MachineState_Running)
		m_Machine.PowerUp();

	return 0;
}

void CMainWindow::UpdateTrayIcon(bool bTextToo, MachineState state)
{
	if (state==MachineState_Null)
	{
		// Get current machine state
		state=m_Machine.GetState();
	}

	m_NotifyIcon.StartUpdate();

	if (bTextToo)
	{
		CUniString str=Format(L"VBoxHeadlessTray - %s - %s", g_strMachineName, GetMachineStateDescription(state));
		DWORD dwPid=m_Machine.GetHeadlessPid();
		if (dwPid!=0)
			str.Append(Format(L" (pid:%i)", dwPid));
		m_NotifyIcon.SetToolTip(str);
	}

	bool bNeedTimer=false;

	HICON hIcon=m_hIconOff;
	switch (state)
	{
		case MachineState_Saved:
			hIcon=m_hIconSaved;
			break;

		case MachineState_Aborted:
		case MachineState_Teleported:
			hIcon=m_hIconAborted;
			break;

		case MachineState_Running:
			hIcon=m_hIconRunning;
			break;

		case MachineState_Paused:
			hIcon=m_hIconPaused;
			break;

		case MachineState_Stuck:
			hIcon=m_hIconAborted;
			break;

		case MachineState_Saving:
		case MachineState_Stopping:
		case MachineState_DeletingSnapshot:
		case MachineState_Teleporting:
		case MachineState_LiveSnapshotting:
		case MachineState_TeleportingPausedVM:
			hIcon=m_hIconTransition[2-m_iAnimationFrame];
			bNeedTimer=true;
			break;

		case MachineState_Restoring:
		case MachineState_Starting:
		case MachineState_SettingUp:
		case MachineState_TeleportingIn:
		case MachineState_RestoringSnapshot:
			hIcon=m_hIconTransition[m_iAnimationFrame];
			bNeedTimer=true;
			break;
	}

	if (bNeedTimer)
		StartTimer();
	else
		StopTimer();

	m_NotifyIcon.SetIcon(hIcon);


	m_NotifyIcon.EndUpdate();
}

void CMainWindow::StartTimer()
{
	if (!m_bTimerRunning)
	{
		SetTimer(1, 300);
		m_bTimerRunning=true;
	}
}

void CMainWindow::StopTimer()
{
	if (m_bTimerRunning)
	{
		KillTimer(1);
		m_bTimerRunning=false;
	}
}


LRESULT CMainWindow::OnNotifyRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SetForegroundWindow(m_hWnd);
	CSmartHandle<HMENU> hMenus=LoadMenu(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDR_CONTEXTMENUS));

	MachineState state=m_Machine.GetState();

	HMENU hMenu=GetSubMenu(hMenus, state<MachineState_Running ? 1 : 0);

	POINT pt;
	GetCursorPos(&pt);

	// Remove redundant commands
	if (state==MachineState_Paused)
	{
		DeleteMenu(hMenus, ID_TRAY_PAUSE, MF_BYCOMMAND);
		EnableMenuItem(hMenus, ID_TRAY_RESET, MF_BYCOMMAND|MF_DISABLED);
		EnableMenuItem(hMenus, ID_TRAY_SHUTDOWN, MF_BYCOMMAND|MF_DISABLED);
		EnableMenuItem(hMenus, ID_TRAY_SLEEP, MF_BYCOMMAND|MF_DISABLED);
	}
	else
	{
		DeleteMenu(hMenus, ID_TRAY_UNPAUSE, MF_BYCOMMAND);
	}

	if (m_Machine.GetHeadlessPid()==0)
	{
		DeleteMenu(hMenus, ID_TRAY_VBOXGUI, MF_BYCOMMAND);
	}
	else
	{
		DeleteMenu(hMenus, ID_TRAY_GOHEADLESS, MF_BYCOMMAND);
	}

	// Disable ACPI commands if additions not installed
	if (!m_Machine.AdditionsActive())
	{
		EnableMenuItem(hMenus, ID_TRAY_SHUTDOWN, MF_BYCOMMAND|MF_DISABLED);
		EnableMenuItem(hMenus, ID_TRAY_SLEEP, MF_BYCOMMAND|MF_DISABLED);
	}

	m_vecCustomCommands.RemoveAll();
	m_vecCustomVerbs.RemoveAll();

	CComPtr<IMachine> spMachine=m_Machine.GetMachine();
	if (!spMachine)
	{
		CComPtr<IVirtualBox> spVirtualBox;
		if (SUCCEEDED(spVirtualBox.CoCreateInstance(__uuidof(VirtualBox))))
			spVirtualBox->FindMachine(CComBSTR(g_strMachineName), &spMachine);
	}

	if (spMachine)
	{
		// Get a list of command names
		CSafeArray Names;
		CSafeArray Values;
		CSafeArray Timestamps;
		CSafeArray Flags;
		if (SUCCEEDED(spMachine->EnumerateGuestProperties(
				CComBSTR("VBoxHeadlessTray\\ContextMenus\\*"),
				&Names, &Values, &Timestamps, &Flags)) && Names.m_pArray!=NULL)
		{
			CUniStringVector vecCommandNames;
			for (long i=Names.GetLBound(); i<=Names.GetUBound(); i++)
			{
				CComBSTR bstr;
				SafeArrayGetElement(Names, &i, &bstr);

				CVector<CUniString> vec;
				SplitString(bstr, L"\\", vec, false);

				if (vec.GetSize()>=3)
				{
					if (vecCommandNames.FindInsensitive(vec[2])<0)
						vecCommandNames.Add(vec[2]);
				}
			}

			// Now just read each property directly
			for (int i=0; i<vecCommandNames.GetSize(); i++)
			{
				CComBSTR bstrMenuText;
				ULONG64 temp;
				CComBSTR bstrTemp;
				spMachine->GetGuestProperty(
					CComBSTR(Format(L"VBoxHeadlessTray\\ContextMenus\\%s\\menutext", vecCommandNames[i])),
					&bstrMenuText, &temp, &bstrTemp);

				bstrTemp.Empty();
				CComBSTR bstrCommand;
				spMachine->GetGuestProperty(
					CComBSTR(Format(L"VBoxHeadlessTray\\ContextMenus\\%s\\command", vecCommandNames[i])),
					&bstrCommand, &temp, &bstrTemp);

				bstrTemp.Empty();
				CComBSTR bstrVerb;
				spMachine->GetGuestProperty(
					CComBSTR(Format(L"VBoxHeadlessTray\\ContextMenus\\%s\\verb", vecCommandNames[i])),
					&bstrVerb, &temp, &bstrTemp);

				bstrTemp.Empty();
				CComBSTR bstrContext;
				spMachine->GetGuestProperty(
					CComBSTR(Format(L"VBoxHeadlessTray\\ContextMenus\\%s\\context", vecCommandNames[i])),
					&bstrContext, &temp, &bstrTemp);


				// Check context
				if ((IsEmptyString(bstrContext) || IsEqualString(bstrContext, L"Running")) && m_Machine.GetState()<MachineState_Running)
					continue;
				else if (IsEqualString(bstrContext, L"Stopped") && m_Machine.GetState()>=MachineState_Running)
					continue;

				InsertMenu(hMenu, GetMenuItemCount(hMenu)-1, MF_BYPOSITION|MF_STRING, ID_CUSTOM_COMMAND_0+m_vecCustomCommands.GetSize(), bstrMenuText);
				
				m_vecCustomCommands.Add(bstrCommand.m_str);
				m_vecCustomVerbs.Add(bstrVerb.m_str);

			}

			if (m_vecCustomCommands.GetSize()>0)
			{
				InsertMenu(hMenu, GetMenuItemCount(hMenu)-1, MF_BYPOSITION|MF_SEPARATOR, -1, NULL);
			}
		}
	}


	TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);

	PostMessage(WM_NULL, 0, 0);
	return 0;
}

// ID_TRAY_EXIT Handler
LRESULT CMainWindow::OnTrayExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	// Turn off auto run
	log("Removing autorun entries\n");
	ManageAutoRun(Format(L"VBoxHeadlessTray %s", g_strMachineName), maroClear, NULL);

	if (m_Machine.GetState()<MachineState_Running || m_Machine.GetHeadlessPid()==0)
	{
		log("Machine stopped or not launched by us, exiting immediately\n");
		PostQuitMessage(0);
	}
	else
	{
		log("Saving state before exit\n");
		m_iSaveStateReason=ID_TRAY_EXIT;
		m_Machine.SaveState();
	}

	return 0;
}

LRESULT CMainWindow::OnTrayPowerOn(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_Machine.PowerUp();
	return 0;
}

LRESULT CMainWindow::OnTrayPowerOff(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (SlxMessageBox(Format(L"Are you sure you want to power down \"%s\"?", g_strMachineName), MB_YESNO|MB_ICONQUESTION)!=IDYES)
		return 0;

	m_Machine.PowerDown();
	return 0;
}

LRESULT CMainWindow::OnTraySaveState(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_Machine.SaveState();
	return 0;
}

LRESULT CMainWindow::OnTrayReset(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (SlxMessageBox(Format(L"Are you sure you want to reset \"%s\"?", g_strMachineName), MB_YESNO|MB_ICONQUESTION)!=IDYES)
		return 0;

	m_Machine.Reset();
	return 0;
}

LRESULT CMainWindow::OnTrayPause(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_Machine.Pause();
	return 0;
}

LRESULT CMainWindow::OnTrayUnPause(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_Machine.Resume();
	return 0;
}


void CMainWindow::OnError(const wchar_t* pszMessage)
{
	SlxMessageBox(pszMessage, MB_OK|MB_ICONHAND);
	return;
}

void CMainWindow::OnStateChange(MachineState newState)
{
	if (newState<MachineState_Running)
	{
		switch (m_iSaveStateReason)
		{
			case ID_TRAY_EXIT:
				log("Machine saved, posting quit message\n");
				PostQuitMessage(0);
				break;

			case ID_TRAY_VBOXGUI:
			case ID_TRAY_GOHEADLESS:
				Sleep(200);
				PostMessage(WM_COMMAND, m_iSaveStateReason);
				break;
		}
		m_iSaveStateReason=0;
	}
	else
	{
		UpdateTrayIcon(true, newState);
	}
}

// ID_TRAY_OPENREMOTEDESKTOPCONNECTION Handler
/*
LRESULT CMainWindow::OnOpenRemoteDesktopConnection(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	// Quit if not running
	if (m_Machine.GetState()!=MachineState_Running)
		return 0;

	CComPtr<IVRDPServer> spVRDPServer;
	m_Machine.GetMachine()->get_VRDPServer(&spVRDPServer);
	if (!spVRDPServer)
		return 0;

	BOOL bEnabled;
	spVRDPServer->get_Enabled(&bEnabled);
	if (!bEnabled)
	{
		SlxMessageBox(L"Remote desktop is not enabled on this virtual machine", MB_OK|MB_ICONINFORMATION);
		return 0;
	}

	// Get net address and port
	CComBSTR bstrNetAddress;
	spVRDPServer->get_NetAddress(&bstrNetAddress);
	ULONG lPort;
	spVRDPServer->get_Port(&lPort);

	if (bstrNetAddress.Length()==0)
		bstrNetAddress=L"localhost";

	WinExec(Format(L"mstsc.exe /v:%s:%i", bstrNetAddress, lPort));

	return 0;
}
*/

// WM_TIMER Handler
LRESULT CMainWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_iAnimationFrame++;
	if (m_iAnimationFrame>2)
		m_iAnimationFrame=0;

	UpdateTrayIcon(false);
	return 0;
}

// WM_QUERYENDSESSION Handler
LRESULT CMainWindow::OnQueryEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	log("OnQueryEndSession: %S\n", GetMachineStateDescription(m_Machine.GetState()));

	if (m_Machine.GetState() >= MachineState_Running)
	{
		log("ShutdownBlockReasonCreate\n");
		SlxShutdownBlockReasonCreate(m_hWnd, Format(L"Shutting down virtual machine %s", g_strMachineName));
		return TRUE;
	}
	else
	{
		return TRUE;	// Shutdown allowed
	}
}

// WM_ENDSESSION Handler
LRESULT CMainWindow::OnEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_Machine.GetState()>=MachineState_Running)
	{
		log("Saving state...\n");
		m_Machine.SaveState();

		log("Waiting...\n");

		// Dispatch messages until machine is closed
		while (m_Machine.GetState()>=MachineState_Running)
		{
			WaitMessage();

			MSG msg;
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) && m_Machine.GetState()>=MachineState_Running)
			{
				if (!IsInputMessage(msg.message))
					DispatchMessage(&msg);
			}
		}

		log("Saved.\n");

		m_NotifyIcon.Delete();
		m_Machine.Close();
	}


	SlxShutdownBlockReasonDestroy(m_hWnd);
	log("Shutdown block released\n");
	return 0;
}

// ID_CUSTOM_COMMAND_0 Handler
LRESULT CMainWindow::OnCustomCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	// Work out index
	int iIndex=wID-ID_CUSTOM_COMMAND_0;
	if (iIndex<0 || iIndex>=m_vecCustomCommands.GetSize())
		return 0;

	if (IsEmptyString(m_vecCustomVerbs[iIndex]))
	{
		// Execute it
		if (!WinExec(m_vecCustomCommands[iIndex]))
		{
			HRESULT hr=HRESULT_FROM_WIN32(GetLastError());
			SlxMessageBox(Format(L"Failed to execute '%s' - %s", m_vecCustomCommands[iIndex], FormatError(hr)), MB_OK|MB_ICONHAND);
		}
	}
	else
	{
		int iError=(int)ShellExecute(NULL, m_vecCustomVerbs[iIndex], m_vecCustomCommands[iIndex], NULL, NULL, SW_SHOWNORMAL);
		if (iError<32)
		{
			SlxMessageBox(Format(L"Failed to %s '%s' (error %i)", m_vecCustomVerbs[iIndex], m_vecCustomCommands[iIndex], iError), MB_OK|MB_ICONHAND);
		}
	}

	return 0;
}

// WM_DESTROY Handler
LRESULT CMainWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_Machine.Close();
	return 0;
}

// ID_TRAY_SHUTDOWN Handler
LRESULT CMainWindow::OnTrayShutdown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_Machine.AcpiPowerButton();
	return 0;
}

// ID_TRAY_SLEEP Handler
LRESULT CMainWindow::OnTraySleep(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_Machine.AcpiSleep();
	return 0;
}

// ID_TRAY_GOHEADLESS Handler
LRESULT CMainWindow::OnTrayGoHeadless(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (m_Machine.GetState()>=MachineState_Running)
	{
		m_Machine.SaveState();
		m_iSaveStateReason=ID_TRAY_GOHEADLESS;
	}
	else
	{
		m_Machine.PowerUp();
	}
	return 0;
}

// ID_TRAY_VBOXGUI Handler
LRESULT CMainWindow::OnTrayVBoxGui(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (m_Machine.GetState()>=MachineState_Running)
	{
		m_Machine.SaveState();
		m_iSaveStateReason=ID_TRAY_VBOXGUI;
	}
	else
	{
		m_Machine.OpenGUI();
	}
	return 0;
}

// ID_TRAY_REMOTEDESKTOP Handler
LRESULT CMainWindow::OnRemoteDesktop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	CComPtr<IVRDPServer> spVrdpServer;
	m_Machine.GetMachine()->get_VRDPServer(&spVrdpServer);
	if (spVrdpServer)
	{
		BOOL bEnabled;
		spVrdpServer->get_Enabled(&bEnabled);
		if (!bEnabled)
		{
			SlxMessageBox(L"Remote display server is not enabled for this machine, use VirtualBox management console to enable this feature", MB_OK|MB_ICONINFORMATION);
			return 0;
		}

		// Get connection settings for VRDP server
		CComBSTR bstrPorts;	  
		spVrdpServer->get_Ports(&bstrPorts);
		CComBSTR bstrNetAddress;
		spVrdpServer->get_NetAddress(&bstrNetAddress);
		if (IsEmptyString(bstrNetAddress))
			bstrNetAddress=L"localhost";

		WinExec(Format(L"mstsc.exe /v:%s:%s", bstrNetAddress, bstrPorts));

	}
	return 0;
}

