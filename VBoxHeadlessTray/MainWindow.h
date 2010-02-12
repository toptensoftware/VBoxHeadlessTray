//////////////////////////////////////////////////////////////////////////
// MainWindow.h - declaration of MainWindow

#ifndef __MAINWINDOW_H
#define __MAINWINDOW_H

#include "VBoxMachine.h"

class CMainWindow : 
	public CWindowImpl<CMainWindow>,
	public IVBoxMachineEvents
{
public:
// Construction
			CMainWindow();
	virtual ~CMainWindow();

// Operations
	bool Create();
	void Destroy();

	void UpdateTrayIcon(bool bTextToo=true, MachineState state=MachineState_Null);
	void StartTimer();
	void StopTimer();

DECLARE_WND_CLASS(_T("VBoxHeadlessTray"));

BEGIN_MSG_MAP(CMainWindow)
	NOTIFYICON_HANDLER(m_NotifyIcon, WM_RBUTTONUP, OnNotifyRButtonUp)
	NOTIFYICON_HANDLER(m_NotifyIcon, WM_LBUTTONDOWN, OnNotifyRButtonUp)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	COMMAND_ID_HANDLER(ID_TRAY_EXIT, OnTrayExit)
	COMMAND_ID_HANDLER(ID_TRAY_POWERON, OnTrayPowerOn)
	COMMAND_ID_HANDLER(ID_TRAY_POWEROFF, OnTrayPowerOff)
	COMMAND_ID_HANDLER(ID_TRAY_SAVESTATE, OnTraySaveState)
	COMMAND_ID_HANDLER(ID_TRAY_RESET, OnTrayReset)
	COMMAND_ID_HANDLER(ID_TRAY_PAUSE, OnTrayPause)
//	COMMAND_ID_HANDLER(ID_TRAY_OPENREMOTEDESKTOPCONNECTION, OnOpenRemoteDesktopConnection)
	MESSAGE_HANDLER(WM_TIMER, OnTimer)
	MESSAGE_HANDLER(WM_QUERYENDSESSION, OnQueryEndSession)
	MESSAGE_HANDLER(WM_ENDSESSION, OnEndSession)
	COMMAND_RANGE_HANDLER(ID_CUSTOM_COMMAND_0, ID_CUSTOM_COMMAND_N, OnCustomCommand)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	COMMAND_ID_HANDLER(ID_TRAY_SHUTDOWN, OnTrayShutdown)
	COMMAND_ID_HANDLER(ID_TRAY_SLEEP, OnTraySleep)
	COMMAND_ID_HANDLER(ID_TRAY_GOHEADLESS, OnTrayGoHeadless)
	COMMAND_ID_HANDLER(ID_TRAY_VBOXGUI, OnTrayVBoxGui)
	COMMAND_ID_HANDLER(ID_TRAY_REMOTEDESKTOP, OnRemoteDesktop)
END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNotifyRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnTrayExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrayPowerOn(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrayPowerOff(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTraySaveState(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrayReset(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrayPause(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrayUnPause(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//	LRESULT OnOpenRemoteDesktopConnection(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnQueryEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCustomCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnTrayShutdown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTraySleep(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrayGoHeadless(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTrayVBoxGui(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnRemoteDesktop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	// Handler Insert Pos(CMainWindow)

	// Create the icon
	CNotifyIcon m_NotifyIcon;
	CSmartHandle<HICON>	m_hIcon;
	CSmartHandle<HICON>	m_hIconOff;
	CSmartHandle<HICON>	m_hIconPaused;
	CSmartHandle<HICON>	m_hIconSaved;
	CSmartHandle<HICON>	m_hIconAborted;
	CSmartHandle<HICON>	m_hIconRunning;
	CSmartHandle<HICON>	m_hIconTransition[3];
	int			m_iSaveStateReason;
	CVBoxMachine	m_Machine;
	bool		m_bTimerRunning;
	int			m_iAnimationFrame;

	CVector<CUniString>	m_vecCustomCommands;
	CVector<CUniString>	m_vecCustomVerbs;

// IVBoxMachineEvents
	virtual void OnError(const wchar_t* pszMessage);
	virtual void OnStateChange(MachineState newState);
};



#endif	// __MAINWINDOW_H

