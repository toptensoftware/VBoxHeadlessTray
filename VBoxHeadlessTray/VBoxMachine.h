//////////////////////////////////////////////////////////////////////////
// VBoxMachine.h - declaration of CVBoxMachine class

#ifndef __VBOXMACHINE_H
#define __VBOXMACHINE_H

#include "Utils.h"

#define _F_INPROC_VM


const wchar_t* GetMachineStateDescription(MachineState s);

class __declspec(uuid("46137EEC-703B-4fe5-AFD4-7C9BBBBA0259")) LibVirtualBox;

#define VMM_MACHINE_STOPPED		(WM_USER + 100)

template <class Base>
class CComObjectNoRef : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectNoRef(void* = NULL){m_hResFinalConstruct = FinalConstruct();}
	~CComObjectNoRef()
	{
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}

	STDMETHOD_(ULONG, AddRef)() {return 2;}
	STDMETHOD_(ULONG, Release)(){return 1;}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
		{return _InternalQueryInterface(iid, ppvObject);}
	HRESULT m_hResFinalConstruct;
};



class CFramebuffer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IFramebuffer, &__uuidof(IFramebuffer), &__uuidof(LibVirtualBox), kTypeLibraryMajorVersion, kTypeLibraryMinorVersion>
{
public:
// Construction
			CFramebuffer();
	virtual ~CFramebuffer();

// Interface map
BEGIN_COM_MAP(CFramebuffer)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IFramebuffer)
END_COM_MAP()

// Operations
	void FreeVRAM();

// IFramebuffer
	STDMETHODIMP get_Address(BYTE * * aAddress);
	STDMETHODIMP get_Width(ULONG * aWidth);
	STDMETHODIMP get_Height(ULONG * aHeight);
	STDMETHODIMP get_BitsPerPixel(ULONG * aBitsPerPixel);
	STDMETHODIMP get_BytesPerLine(ULONG * aBytesPerLine);
	STDMETHODIMP get_PixelFormat(ULONG * aPixelFormat);
	STDMETHODIMP get_UsesGuestVRAM(BOOL * aUsesGuestVRAM);
	STDMETHODIMP get_HeightReduction(ULONG * aHeightReduction);
	STDMETHODIMP get_Overlay(IFramebufferOverlay * * aOverlay);
	STDMETHODIMP get_WinId(ULONG64 * aWinId);
	STDMETHODIMP Lock();
	STDMETHODIMP Unlock();
	STDMETHODIMP NotifyUpdate(
        ULONG aX,
        ULONG aY,
        ULONG aWidth,
        ULONG aHeight
    );
	STDMETHODIMP RequestResize(
        ULONG aScreenId,
        ULONG aPixelFormat,
        BYTE * aVRAM,
        ULONG aBitsPerPixel,
        ULONG aBytesPerLine,
        ULONG aWidth,
        ULONG aHeight,
        BOOL * aFinished
    );
	STDMETHODIMP VideoModeSupported(
        ULONG aWidth,
        ULONG aHeight,
        ULONG aBpp,
        BOOL * aSupported
    );
	STDMETHODIMP GetVisibleRegion(
        BYTE * aRectangles,
        ULONG aCount,
        ULONG * aCountCopied
    );
	STDMETHODIMP SetVisibleRegion(
        BYTE * aRectangles,
        ULONG aCount
    );
	STDMETHODIMP ProcessVHWACommand(
        BYTE * aCommand
	);

	BYTE*	m_pVRAM;
	ULONG	m_nWidth;
	ULONG	m_nHeight;
	ULONG	m_nBPP;
	ULONG	m_nBytesPerLine;
	ULONG	m_nPixelFormat;
	BOOL	m_bUsesGuestVRAM;
	CCriticalSection	m_cs;
};




// Machine events
class IVBoxMachineEvents
{
public:
	virtual void OnError(const wchar_t* pszMessage)=0;
	virtual void OnStateChange(MachineState newState)=0;
};

// CVBoxMachine Class
class CVBoxMachine : 
	public CPostableObject


{
public:
// Construction
			CVBoxMachine();
	virtual ~CVBoxMachine();

// Attributes
	void SetMachineName(const wchar_t* psz);
	CUniString GetMachineName();

	void SetEventHandler(IVBoxMachineEvents* pEvents);

	MachineState GetState();
	CUniString GetErrorMessage();

	void Close();
	bool PowerUp();
	bool PowerDown();
	bool SaveState();
	bool Pause();
	bool Resume();
	bool Reset();
	bool IsOpen() { return m_spVirtualBox!=NULL; }

	IConsole* GetConsole() { return m_spConsole; };
	IMachine* GetMachine() { return m_spMachine; };


// Operations

// Implementation
protected:
// Attributes
	CComPtr<IVirtualBox>	m_spVirtualBox;
	CComPtr<ISession>		m_spSession;
	CComPtr<IMachine>		m_spMachine;
	CComPtr<IConsole>		m_spConsole;
	CComBSTR				m_bstrMachineID;
	CUniString				m_strMachineName;
	CUniString				m_strLastError;
	IVBoxMachineEvents*		m_pEvents;
	MachineState			m_State;


	bool SetError(const wchar_t* psz);

// Operations
	class CConsoleEvents : 
		public CComObjectRootEx<CComSingleThreadModel>,
		public IDispatchImpl<IConsoleCallback, &__uuidof(IConsoleCallback), &__uuidof(LibVirtualBox), kTypeLibraryMajorVersion, kTypeLibraryMinorVersion>
	{
	public:

	BEGIN_COM_MAP(CConsoleEvents)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IConsoleCallback)
	END_COM_MAP()

		STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
				LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
				EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{
			return __super::Invoke(dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr);
		}

		STDMETHODIMP OnMousePointerShapeChange(
			BOOL aVisible,
			BOOL aAlpha,
			ULONG aXHot,
			ULONG aYHot,
			ULONG aWidth,
			ULONG aHeight,
			BYTE * aShape
		)
		{
			return S_OK;
		};
		STDMETHODIMP OnMouseCapabilityChange(
			BOOL aSupportsAbsolute,
			BOOL aNeedsHostCursor
		)
		{
			CVBoxMachine* pThis=OUTERCLASS(CVBoxMachine, m_ConsoleCallback);
			if (aSupportsAbsolute && pThis->m_spConsole)
			{
				CComPtr<IMouse> spMouse;
				pThis->m_spConsole->get_Mouse(&spMouse);
				if (spMouse)
				{
					spMouse->PutMouseEventAbsolute(-1, -1, 0, 0);
				}
			}
			return S_OK;
		};
		STDMETHODIMP OnKeyboardLedsChange(
			BOOL aNumLock,
			BOOL aCapsLock,
			BOOL aScrollLock
		)
		{
			return S_OK;
		};
		STDMETHODIMP OnStateChange(
			MachineState aState
		)
		{
			CVBoxMachine* pThis=OUTERCLASS(CVBoxMachine, m_ConsoleCallback);
			log("State change: %S\n", GetMachineStateDescription(aState));
			pThis->m_State=aState;
			pThis->m_pEvents->OnStateChange(aState);
			if (aState<MachineState_Running)
			{
				pThis->Post(VMM_MACHINE_STOPPED);
			}
			return S_OK;
		};
		STDMETHODIMP OnAdditionsStateChange()
		{
			return S_OK;
		};
		STDMETHODIMP OnDVDDriveChange()
		{
			return S_OK;
		};
		STDMETHODIMP OnFloppyDriveChange()
		{
			return S_OK;
		};
		STDMETHODIMP OnNetworkAdapterChange(
			INetworkAdapter * aNetworkAdapter
		)
		{
			return S_OK;
		};
		STDMETHODIMP OnSerialPortChange(
			ISerialPort * aSerialPort
		)
		{
			return S_OK;
		};
		STDMETHODIMP OnParallelPortChange(
			IParallelPort * aParallelPort
		)
		{
			return S_OK;
		};
		STDMETHODIMP OnStorageControllerChange()
		{
			return S_OK;
		};
		STDMETHODIMP OnVRDPServerChange()
		{
			return S_OK;
		};
		STDMETHODIMP OnUSBControllerChange()
		{
			return S_OK;
		};
		STDMETHODIMP OnUSBDeviceStateChange(
			IUSBDevice * aDevice,
			BOOL aAttached,
			IVirtualBoxErrorInfo * aError
		)
		{
			return S_OK;
		};
		STDMETHODIMP OnSharedFolderChange(
			Scope aScope
		)
		{
			return S_OK;
		};
		STDMETHODIMP OnRuntimeError(
			BOOL aFatal,
			BSTR aId,
			BSTR aMessage
		)
		{
			return S_OK;
		};
		STDMETHODIMP OnCanShowWindow(
			BOOL * aCanShow
		)
		{
			if (!aCanShow)
				return E_POINTER;
			/* Headless windows should not be shown */
			*aCanShow = FALSE;
			return S_OK;
		};
		STDMETHODIMP OnShowWindow(
			ULONG64 * aWinId
		)
		{
			ASSERT(FALSE);
			if (!aWinId)
				return E_POINTER;
			*aWinId = 0;
			return E_NOTIMPL;
		};
	};

	CComObjectNoRef<CConsoleEvents>	m_ConsoleCallback;

#ifndef _F_INPROC_VM
	HRESULT StartHeadlessProcess();
#endif
	bool HandleProgress(const wchar_t* pszOp, HRESULT hr, IProgress* pProgress);
	bool HandleResult(const wchar_t* pszOp, HRESULT hr);
	virtual LRESULT OnMessage(UINT nMessage, WPARAM wParam, LPARAM lParam);
};

#endif	// __VBOXMACHINE_H

