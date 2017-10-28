// SelectMachineDlg.cpp : Implementation of CSelectMachineDlg
#include "stdafx.h"
#include "VBoxHeadlessTray.h"

#include "SelectMachineDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectMachineDlg

// Constructor
CSelectMachineDlg::CSelectMachineDlg()
{
}

// Destructor
CSelectMachineDlg::~CSelectMachineDlg()
{
}


/////////////////////////////////////////////////////////////////////////////
// Message handlers

// WM_INITDIALOG handler
LRESULT CSelectMachineDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CenterWindow();

	RefreshList();

	return 1;  // Let the system set the focus
}

HRESULT CSelectMachineDlg::RefreshList()
{
	ATLControls::CListBox cb(GetDlgItem(IDC_MACHINENAME));
	cb.ResetContent();

	HRESULT rc;

	CoInitialize(NULL);

	CComPtr<IVirtualBoxClient> virtualBoxClient;

    rc = CoCreateInstance(CLSID_VirtualBoxClient, /* the VirtualBoxClient object */
                                  NULL,                   /* no aggregation */
                                  CLSCTX_INPROC_SERVER,   /* the object lives in the current process */
                                  IID_IVirtualBoxClient,  /* IID of the interface */
                                  (void**)&virtualBoxClient);
	RETURNIFFAILED(rc);

	CComPtr<IVirtualBox> spVirtualBox;

    RETURNIFFAILED(virtualBoxClient->get_VirtualBox(&spVirtualBox));

	SAFEARRAY *machinesArray = NULL;

	RETURNIFFAILED(spVirtualBox->get_Machines(&machinesArray));

	CComPtrVector<IMachine*> vecMachines;

	vecMachines.InitFromSafeArray(machinesArray);

	for (int i=0; i<vecMachines.GetSize(); i++)
	{
		CComBSTR bstrName;
		if (SUCCEEDED(vecMachines[i]->get_Name(&bstrName)))
		{
			cb.AddString(bstrName);
		}
	}

	cb.SetCurSel(0);

	CheckDlgButton(IDC_POWERONMACHINE, m_bPowerOnMachine);

	return S_OK;
}

// IDOK button handler
LRESULT CSelectMachineDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	ATLControls::CListBox cb(GetDlgItem(IDC_MACHINENAME));
	int iSel=cb.GetCurSel();
	if (iSel<0)
		return 0;

	CComBSTR bstrMachineName;
	cb.GetTextBSTR(iSel, bstrMachineName.m_str);
	if (IsEmptyString(bstrMachineName))
		return 0;

	m_strMachineName=bstrMachineName;
	m_bPowerOnMachine=!!IsDlgButtonChecked(IDC_POWERONMACHINE);

	EndDialog(wID);
	return 0;
}

// IDCANCEL button handler
LRESULT CSelectMachineDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	EndDialog(wID);
	return 0;
}

// IDC_MACHINENAME LBN_DBLCLK Handler
LRESULT CSelectMachineDlg::OnListDblClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	PostMessage(WM_COMMAND, IDOK);
	return 0;
}

