//////////////////////////////////////////////////////////////////////////
// VBoxMachine.h - declaration of CVBoxMachine class

#ifndef __VBOXMACHINE_H
#define __VBOXMACHINE_H

#include "Utils.h"

const wchar_t* GetMachineStateDescription(MachineState s);

class __declspec(uuid("46137EEC-703B-4fe5-AFD4-7C9BBBBA0259")) LibVirtualBox;

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
class CVBoxMachine
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

	HRESULT Open();
	void Close();
	bool PowerUp();
	bool PowerDown();
	bool SaveState();
	bool Pause();
	bool Resume();
	bool Reset();
	bool IsOpen() { return m_spVirtualBox!=NULL; }

	IMachine* GetMachine() { return m_spMachine; };
	DWORD GetHeadlessPid() { return m_dwHeadlessPid; }


// Operations

// Implementation
protected:
// Attributes
	CComPtr<IVirtualBox>	m_spVirtualBox;
	CComPtr<IMachine>		m_spMachine;
	CComBSTR				m_bstrMachineID;
	CUniString				m_strMachineName;
	CUniString				m_strLastError;
	IVBoxMachineEvents*		m_pEvents;
	MachineState			m_State;
	bool					m_bCallbackRegistered;
	DWORD					m_dwHeadlessPid;
	HANDLE					m_hHeadlessProcess;

	bool SetError(const wchar_t* psz);
	void OnMachineStateChange(const wchar_t* pszMachineID, MachineState State);
	HRESULT Exec(const wchar_t* pszCommandLine, DWORD* ppid=NULL, HANDLE* phProcess=NULL);

// Operations

	class CVirtualBoxEvents : 
		public CComObjectRootEx<CComSingleThreadModel>,
		public IDispatchImpl<IVirtualBoxCallback, &__uuidof(IVirtualBoxCallback), &__uuidof(LibVirtualBox), kTypeLibraryMajorVersion, kTypeLibraryMinorVersion>
	{
	public:
		BEGIN_COM_MAP(CVirtualBoxEvents)
			COM_INTERFACE_ENTRY(IDispatch)
			COM_INTERFACE_ENTRY(IVirtualBoxCallback)
		END_COM_MAP()


        virtual HRESULT STDMETHODCALLTYPE OnMachineStateChange( 
            /* [in] */ BSTR aMachineId,
            /* [in] */ MachineState aState) 
		{
			CVBoxMachine* pThis=OUTERCLASS(CVBoxMachine, m_VirtualBoxEvents);
			pThis->OnMachineStateChange(aMachineId, aState);
			return E_NOTIMPL; 
		};
        
        virtual HRESULT STDMETHODCALLTYPE OnMachineDataChange( 
            /* [in] */ BSTR aMachineId) { return E_NOTIMPL; };
        
        virtual HRESULT STDMETHODCALLTYPE OnExtraDataCanChange( 
            /* [in] */ BSTR aMachineId,
            /* [in] */ BSTR aKey,
            /* [in] */ BSTR aValue,
            /* [out] */ BSTR *aError,
            /* [retval][out] */ BOOL *aAllowChange) { return E_NOTIMPL; };
        
        virtual HRESULT STDMETHODCALLTYPE OnExtraDataChange( 
            /* [in] */ BSTR aMachineId,
            /* [in] */ BSTR aKey,
            /* [in] */ BSTR aValue) { return E_NOTIMPL; };
        
        virtual HRESULT STDMETHODCALLTYPE OnMediumRegistered( 
            /* [in] */ BSTR aMediumId,
            /* [in] */ DeviceType aMediumType,
            /* [in] */ BOOL aRegistered) { return E_NOTIMPL; };
        
        virtual HRESULT STDMETHODCALLTYPE OnMachineRegistered( 
            /* [in] */ BSTR aMachineId,
            /* [in] */ BOOL aRegistered) { return E_NOTIMPL; };
        
        virtual HRESULT STDMETHODCALLTYPE OnSessionStateChange( 
            /* [in] */ BSTR aMachineId,
            /* [in] */ SessionState aState) { return E_NOTIMPL; };
        
        virtual HRESULT STDMETHODCALLTYPE OnSnapshotTaken( 
            /* [in] */ BSTR aMachineId,
            /* [in] */ BSTR aSnapshotId) { return E_NOTIMPL; };
        
        virtual HRESULT STDMETHODCALLTYPE OnSnapshotDiscarded( 
            /* [in] */ BSTR aMachineId,
            /* [in] */ BSTR aSnapshotId) { return E_NOTIMPL; };
        
        virtual HRESULT STDMETHODCALLTYPE OnSnapshotChange( 
            /* [in] */ BSTR aMachineId,
            /* [in] */ BSTR aSnapshotId) { return E_NOTIMPL; };
        
        virtual HRESULT STDMETHODCALLTYPE OnGuestPropertyChange( 
            /* [in] */ BSTR aMachineId,
            /* [in] */ BSTR aName,
            /* [in] */ BSTR aValue,
            /* [in] */ BSTR aFlags) { return E_NOTIMPL; };
	};


	CComObjectNoRef<CVirtualBoxEvents>	m_VirtualBoxEvents;

	HRESULT StartHeadlessProcess();

};

#endif	// __VBOXMACHINE_H

