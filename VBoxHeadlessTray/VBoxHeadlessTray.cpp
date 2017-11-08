// VBoxHeadlessTray.cpp : Implementation of WinMain


#include "stdafx.h"
#include "VBoxHeadlessTray.h"
#include "MainWindow.h"
#include "SelectMachineDlg.h"

// Use Common Controls 6 - don't forget to call InitCommonControls or not even standard controls will work
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

CUniString g_strMachineName;
bool g_bPowerOnMachine=true;

class CVBoxHeadlessTrayModule : public CAtlExeModuleT< CVBoxHeadlessTrayModule >
{
public :
	HRESULT Run(int nShowCmd);
};

CVBoxHeadlessTrayModule _AtlModule;


HRESULT CVBoxHeadlessTrayModule::Run(int nShowCmd)
{
	// Create the main window
	CMainWindow MainWindow;
	if (!MainWindow.Create())
	{
		return E_FAIL;
	}


	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message==WM_USER && msg.hwnd==NULL)
		{
			int i=3;
			ASSERT(FALSE);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	MainWindow.Destroy();

	return S_OK;
}


void ShowHelp()
{
	SlxMessageBox(L"VBoxHeadlessTray © 2009 Topten Software.  All Rights Reserved\n\n"
					L"Usage: VBoxHeadlessTray [-?|-h] [-np] <machinename>\n\n"
					L"  -np: don't power on the machine\n"
					L"  -h:  show this help\n"
					, MB_OK|MB_ICONINFORMATION);
	return;
}

#include <io.h>
#include <fcntl.h>

//
extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, 
                                LPTSTR lpCmdLine, int nShowCmd)
{
	OleInitialize(NULL);
#if 0
   // Allocate a new console window
   AllocConsole();

   // Connect CRT stdout to console
   *stdout = *_fdopen(_open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT), "w" );
   setvbuf( stdout, NULL, _IONBF, 0 );

   // Connect CRT stderr to console
   *stderr = *_fdopen(_open_osfhandle((intptr_t)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT), "w" );
   setvbuf( stderr, NULL, _IONBF, 0 ); 
#endif

	// Shutdown before VBoxSvc
	SetProcessShutdownParameters(0x400, 0);

	// Split command line
	CVector<CUniString> args;
	SplitCommandLine(GetCommandLine(), args);
	args.RemoveAt(0);		// exe path

	// Expand response files
	CUniString strError;
	if (!ExpandResponseFiles(args, strError))
	{
		SlxMessageBox(Format(L"%s\n", strError), MB_OK|MB_ICONHAND);
		return 7;
	}

	// Parse all args
	for (int i=0; i<args.GetSize(); i++)
	{
		CUniString strSwitch, strValue;
		if (ParseArg(args[i], strSwitch, strValue))
		{
			if (IsEqualString(strSwitch, L"?") || IsEqualString(strSwitch, L"h"))
			{
				ShowHelp();
				return 7;
			}
			else if (IsEqualString(strSwitch, L"np"))
			{
				g_bPowerOnMachine=false;
			}
			else
			{
				SlxMessageBox(Format(L"Unrecognised switch: %s\n", args[i]), MB_OK|MB_ICONHAND);
				return 7;
			}
		}
		else
		{
			if (!IsEmptyString(g_strMachineName))
			{
				SlxMessageBox(L"Error, multiple machine names specified", MB_OK|MB_ICONHAND);
				return 7;
			}

			// Store machine name
			g_strMachineName=args[i];
		}
	}

	// Prompt for machine name if not specified on command line
	if (IsEmptyString(g_strMachineName))
	{
		CSelectMachineDlg dlg;
		dlg.m_bPowerOnMachine=g_bPowerOnMachine;
		if (dlg.DoModal()!=IDOK)
			return 7;
		g_strMachineName=dlg.m_strMachineName;
		g_bPowerOnMachine=dlg.m_bPowerOnMachine;
	}


    return _AtlModule.WinMain(nShowCmd);
}

