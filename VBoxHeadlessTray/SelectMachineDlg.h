// SelectMachineDlg.h : Declaration of the CSelectMachineDlg class

#ifndef __SELECTMACHINEDLG_H_
#define __SELECTMACHINEDLG_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSelectMachineDlg
class CSelectMachineDlg : 
	public CDialogImpl<CSelectMachineDlg>
{
public:
			CSelectMachineDlg();
	virtual ~CSelectMachineDlg();

	enum { IDD = IDD_SELECTMACHINEDLG };

	CUniString m_strMachineName;
	bool		m_bPowerOnMachine;

BEGIN_MSG_MAP(CSelectMachineDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_HANDLER(IDC_MACHINENAME, LBN_DBLCLK, OnListDblClick)
END_MSG_MAP()
	LRESULT OnListDblClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	// Handler Insert Pos(CSelectMachineDlg)
// Handler prototypes:	 			
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);


	HRESULT RefreshList();
};

#endif //__SELECTMACHINEDLG_H_
			