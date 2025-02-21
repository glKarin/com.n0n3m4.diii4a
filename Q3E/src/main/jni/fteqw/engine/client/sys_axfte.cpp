#include "quakedef.h"

#ifdef _WIN32
#include "sys_plugfte.h"

#include <windows.h>
#include <objsafe.h>	/*IObjectSafety*/
#include <mshtmdid.h>	/*DISPID_SECURITYCTX*/

#include <olectl.h> /*common dispid values*/

#ifndef DISPID_READYSTATE
/*my oldctl.h is too old*/
#define DISPID_READYSTATE -525
#endif
#ifndef __IOleInPlaceObjectWindowless_INTERFACE_DEFINED__
/*mshtmdid.h didn't declare this, so fall back*/
#define IID_IOleInPlaceObjectWindowless IID_IOleInPlaceObject
#define IOleInPlaceObjectWindowless IOleInPlaceObject
#endif

#ifndef __IOleInPlaceSiteWindowless_INTERFACE_DEFINED__
#define IOleInPlaceSiteWindowless IOleInPlaceSite
#define IID_IOleInPlaceSiteWindowless IID_IOleInPlaceSite
#endif

const GUID axfte_iid = {0x7d676c9f, 0xfb84, 0x40b6, {0xb3, 0xff, 0xe1, 0x08, 0x31, 0x55, 0x7e, 0xeb}};
#define axfte_iid_str "7d676c9f-fb84-40b6-b3ff-e10831557eeb"
extern "C"
{
	extern HINSTANCE	global_hInstance;
}

#ifdef _MSC_VER
#pragma warning(disable:4584) /*shush now*/
#endif

class axfte : public IDispatch, public IClassFactory, public IObjectSafety, 
	public IOleObject, public IOleInPlaceObjectWindowless, public IViewObject, public IPersistPropertyBag2
{
private:
	unsigned int ref;
	IUnknown *site;
	struct context *plug;
	const struct plugfuncs *funcs;
	HWND phwnd;
	static const struct browserfuncs axbrowserfuncs;

public:
	axfte()
	{
		ref = 0;
		site = NULL;
		phwnd = NULL;
		funcs = Plug_GetFuncs(PLUG_APIVER);
		plug = funcs->CreateContext(this, &axbrowserfuncs);
	}
	~axfte()
	{
		funcs->DestroyContext(plug);
		if (site)
			site->Release();
		site = NULL;
	}
	static void statuschanged(void *arg)
	{
		//potentially comes from another thread
		//axfte *fte = (axfte*)arg;
		InvalidateRect(NULL, NULL, FALSE);
	}

	/*IUnknown*/
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		*ppvObject = NULL;
		if (riid == IID_IUnknown)
		{
			*ppvObject = (IUnknown*)(IDispatch*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}
		else if (riid == IID_IDispatch)
		{
			*ppvObject = (IDispatch*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}
		else if (riid == IID_IClassFactory)
		{
			*ppvObject = (IClassFactory*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}
		else if (riid == IID_IObjectSafety)
		{
			*ppvObject = (IObjectSafety*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}
/*		else if (riid == IID_IPersistPropertyBag2)
		{
			*ppvObject = (IPersistPropertyBag2*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}*/
		else if (riid == IID_IOleObject)
		{
			*ppvObject = (IOleObject*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}
		else if (riid == IID_IOleInPlaceObject)
		{
			*ppvObject = (IOleInPlaceObject*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}
		else if (riid == IID_IOleInPlaceObjectWindowless)
		{
			*ppvObject = (IOleInPlaceObjectWindowless*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}

		else if (riid == IID_IOleWindow)
		{
			*ppvObject = (IOleWindow*)(IOleInPlaceObject*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}
		else if (riid == IID_IOleInPlaceObject)
		{
			*ppvObject = (IOleInPlaceObject*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}
		else if (riid == IID_IViewObject)
		{
			*ppvObject = (IViewObject*)this;
			((LPUNKNOWN)*ppvObject)->AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef( void)
	{
		return ++ref;
	}

	virtual ULONG STDMETHODCALLTYPE Release( void)
	{
		if (ref == 1)
		{
			delete this;
			return 0;
		}
		return --ref;
	}






	/*IDispatch*/
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
		/* [out] */ UINT *pctinfo)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
		/* [in] */ UINT iTInfo,
		/* [in] */ LCID lcid,
		/* [out] */ ITypeInfo **ppTInfo)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
		/* [in] */ REFIID riid,
		/* [size_is][in] */ LPOLESTR *rgszNames,
		/* [in] */ UINT cNames,
		/* [in] */ LCID lcid,
		/* [size_is][out] */ DISPID *rgDispId)
	{
		char tmp[1024];
		HRESULT ret = S_OK;
		UINT i;
		int prop;
		for (i = 0; i < cNames; i++)
		{
			wcstombs(tmp, rgszNames[i], sizeof(tmp));
			prop = funcs->FindProp(plug, tmp);
			if (prop >= 0)
			{
				rgDispId[i] = prop;
			}
			else if (!stricmp(tmp, "unselectable"))
				rgDispId[i] = 5001;
			else
			{
				rgDispId[i] = DISPID_UNKNOWN;
				ret = DISP_E_UNKNOWNNAME;
			}
		}
		return ret;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
		/* [in] */ DISPID dispIdMember,
		/* [in] */ REFIID riid,
		/* [in] */ LCID lcid,
		/* [in] */ WORD wFlags,
		/* [out][in] */ DISPPARAMS *pDispParams,
		/* [out] */ VARIANT *pVarResult,
		/* [out] */ EXCEPINFO *pExcepInfo,
		/* [out] */ UINT *puArgErr)
	{
		if(wFlags & DISPATCH_METHOD)
		{
			MessageBox(NULL, "", "invoke method!", 0);
			return DISP_E_MEMBERNOTFOUND;
		}
		else if (wFlags & DISPATCH_PROPERTYGET)
		{
			VariantClear(pVarResult);
			switch(dispIdMember)
			{
			case DISPID_READYSTATE:
				pVarResult->vt = VT_INT;
				pVarResult->intVal = READYSTATE_COMPLETE;
				break;
			case DISPID_ENABLED:
				return DISP_E_MEMBERNOTFOUND;
			case DISPID_SECURITYCTX:
				return DISP_E_MEMBERNOTFOUND;
			default:
				if (dispIdMember >= 0 && dispIdMember < 1000)
				{
					const char *tmpa;
					wchar_t tmpw[1024];
					if (funcs->GetFloat(plug, dispIdMember, &pVarResult->fltVal))
						pVarResult->vt = VT_R4;
					else if (funcs->GetInteger(plug, dispIdMember, &pVarResult->intVal))
						pVarResult->vt = VT_I4;
					else if (funcs->GetString(plug, dispIdMember, &tmpa))
					{
						mbstowcs(tmpw, tmpa, sizeof(tmpw)/sizeof(tmpw[0]));
						funcs->GotString(tmpa);
						pVarResult->vt = VT_BSTR;
						pVarResult->bstrVal = SysAllocString(tmpw);
					}
					else
						return DISP_E_MEMBERNOTFOUND;
				}
				else
				{
					char tmp[1024];
					sprintf(tmp, "DISPATCH_PROPERTYGET dispIdMember=%i", (unsigned int)dispIdMember);
					OutputDebugStringA(tmp);
					return DISP_E_MEMBERNOTFOUND;
				}
			}
		}
		else if (wFlags & DISPATCH_PROPERTYPUT)
		{
			if (dispIdMember >= 0 && dispIdMember < 1000)
			{
				VARIANT *v = &pDispParams->rgvarg[0];
				switch(v->vt)
				{
				case VT_R4:
					funcs->SetFloat(plug, dispIdMember, v->fltVal);
					break;
				case VT_R8:
					funcs->SetFloat(plug, dispIdMember, v->dblVal);
					break;
				case VT_INT:
				case VT_I4:
					funcs->SetInteger(plug, dispIdMember, v->intVal);
					break;
				case VT_BSTR:
					funcs->SetWString(plug, dispIdMember, v->bstrVal);
					break;
				default:
					return DISP_E_TYPEMISMATCH;
				}
				return S_OK;
			}
			else
			{
				char tmp[1024];
				sprintf(tmp, "DISPATCH_PROPERTYPUT dispIdMember=%i", (unsigned int)dispIdMember);
				OutputDebugStringA(tmp);
				return DISP_E_MEMBERNOTFOUND;
			}
		}
		else if (wFlags & DISPATCH_PROPERTYPUTREF)
		{
			char tmp[1024];
			sprintf(tmp, "DISPATCH_PROPERTYPUTREF dispIdMember=%i", (unsigned int)dispIdMember);
			OutputDebugStringA(tmp);
			return DISP_E_MEMBERNOTFOUND;
		}
		else
			return DISP_E_MEMBERNOTFOUND;

		return S_OK;
	}


	/*IClassFactory*/
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateInstance( 
		/* [unique][in] */ IUnknown *pUnkOuter,
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void **ppvObject)
	{
		HRESULT res;

		if (pUnkOuter)
			return CLASS_E_NOAGGREGATION;

		axfte *newaxfte = new axfte();
		res = newaxfte->QueryInterface(riid, ppvObject);
		if (!*ppvObject)
			delete newaxfte;
		return res;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE LockServer( 
		/* [in] */ BOOL fLock)
	{
		return S_OK;
	}

	/*IObjectSafety*/
	virtual HRESULT STDMETHODCALLTYPE GetInterfaceSafetyOptions( 
		/* [in] */ REFIID riid,
		/* [out] */ DWORD *pdwSupportedOptions,
		/* [out] */ DWORD *pdwEnabledOptions)
	{
		*pdwSupportedOptions = *pdwEnabledOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
		return S_OK;
	}
	virtual HRESULT STDMETHODCALLTYPE SetInterfaceSafetyOptions( 
		/* [in] */ REFIID riid,
		/* [in] */ DWORD dwOptionSetMask,
		/* [in] */ DWORD dwEnabledOptions)
	{
		return S_OK;
	}

	/*IOleWindow*/
	virtual /* [input_sync] */ HRESULT STDMETHODCALLTYPE GetWindow( 
		/* [out] */ HWND *phwnd) 
	{
		*phwnd = NULL;
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp( 
		/* [in] */ BOOL fEnterMode)
	{
		return E_NOTIMPL;
	}

	/*IOleInPlaceObject*/
	virtual HRESULT STDMETHODCALLTYPE InPlaceDeactivate( void)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE UIDeactivate( void)
	{
		return E_NOTIMPL;
	}

	virtual /* [input_sync] */ HRESULT STDMETHODCALLTYPE SetObjectRects( 
		/* [in] */ LPCRECT lprcPosRect,
		/* [in] */ LPCRECT lprcClipRect)
	{
		if (phwnd)
			funcs->ChangeWindow(plug, phwnd, lprcPosRect->left, lprcPosRect->top, lprcPosRect->right - lprcPosRect->left, lprcPosRect->bottom - lprcPosRect->top);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ReactivateAndUndo( void)
	{
		return E_NOTIMPL;
	}

	/*IOleObject*/
	virtual HRESULT STDMETHODCALLTYPE SetClientSite( 
		/* [unique][in] */ IOleClientSite *pClientSite)
	{
		IUnknown *osite = site;
		site = pClientSite;
		if (site)
			site->AddRef();

		IOleInPlaceSiteWindowless *oipc;
		if (site)
		if (!FAILED(site->QueryInterface(IID_IOleInPlaceSiteWindowless, (void**)&oipc)))
		{
			IOleInPlaceFrame *pframe;
			IOleInPlaceUIWindow *pdoc;
			RECT posrect;
			RECT cliprect;
			OLEINPLACEFRAMEINFO frameinfo;
			memset(&frameinfo, 0, sizeof(frameinfo));
			frameinfo.cb = sizeof(frameinfo);
			oipc->GetWindowContext(&pframe, &pdoc, &posrect, &cliprect, &frameinfo);
			if (pframe) pframe->Release();
			if (pdoc) pdoc->Release();
			phwnd = frameinfo.hwndFrame;
			oipc->Release();
		}

		if (osite)
			osite->Release();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetClientSite( 
		/* [out] */ IOleClientSite **ppClientSite)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SetHostNames( 
		/* [in] */ LPCOLESTR szContainerApp,
		/* [unique][in] */ LPCOLESTR szContainerObj)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Close( 
		/* [in] */ DWORD dwSaveOption)
	{
		funcs->SetInteger(plug, funcs->FindProp(plug, "running"), 0);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SetMoniker( 
		/* [in] */ DWORD dwWhichMoniker,
		/* [unique][in] */ IMoniker *pmk)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMoniker( 
		/* [in] */ DWORD dwAssign,
		/* [in] */ DWORD dwWhichMoniker,
		/* [out] */ IMoniker **ppmk)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE InitFromData( 
		/* [unique][in] */ IDataObject *pDataObject,
		/* [in] */ BOOL fCreation,
		/* [in] */ DWORD dwReserved)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetClipboardData( 
		/* [in] */ DWORD dwReserved,
		/* [out] */ IDataObject **ppDataObject)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE DoVerb( 
		/* [in] */ LONG iVerb,
		/* [unique][in] */ LPMSG lpmsg,
		/* [unique][in] */ IOleClientSite *pActiveSite,
		/* [in] */ LONG lindex,
		/* [in] */ HWND hwndParent,
		/* [unique][in] */ LPCRECT lprcPosRect)
	{
		switch(iVerb)
		{
		case OLEIVERB_INPLACEACTIVATE:
			IOleInPlaceSiteWindowless *oipc;
			if (!FAILED(pActiveSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void**)&oipc)))
			{
				IOleInPlaceFrame *pframe;
				IOleInPlaceUIWindow *pdoc;
				RECT posrect;
				RECT cliprect;
				OLEINPLACEFRAMEINFO frameinfo;
				memset(&frameinfo, 0, sizeof(frameinfo));
				frameinfo.cb = sizeof(frameinfo);
				oipc->GetWindowContext(&pframe, &pdoc, &posrect, &cliprect, &frameinfo);
				if (pframe) pframe->Release();
				if (pdoc) pdoc->Release();

				phwnd = frameinfo.hwndFrame;
				funcs->ChangeWindow(plug, frameinfo.hwndFrame, lprcPosRect->left, lprcPosRect->top, lprcPosRect->right - lprcPosRect->left, lprcPosRect->bottom - lprcPosRect->top);
				#ifndef __IOleInPlaceSiteWindowless_INTERFACE_DEFINED__
				oipc->OnInPlaceActivate();
				#else
				oipc->OnInPlaceActivateEx(NULL, 1);
				#endif
				oipc->Release();
			}
			break;
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumVerbs( 
		/* [out] */ IEnumOLEVERB **ppEnumOleVerb)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Update( void)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE IsUpToDate( void)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetUserClassID( 
		/* [out] */ CLSID *pClsid)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetUserType( 
		/* [in] */ DWORD dwFormOfType,
		/* [out] */ LPOLESTR *pszUserType)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SetExtent( 
		/* [in] */ DWORD dwDrawAspect,
		/* [in] */ SIZEL *psizel)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetExtent( 
		/* [in] */ DWORD dwDrawAspect,
		/* [out] */ SIZEL *psizel)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Advise( 
		/* [unique][in] */ IAdviseSink *pAdvSink,
		/* [out] */ DWORD *pdwConnection)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Unadvise( 
		/* [in] */ DWORD dwConnection)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumAdvise( 
		/* [out] */ IEnumSTATDATA **ppenumAdvise)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMiscStatus( 
		/* [in] */ DWORD dwAspect,
		/* [out] */ DWORD *pdwStatus)
	{
		*pdwStatus = OLEMISC_RECOMPOSEONRESIZE;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetColorScheme( 
		/* [in] */ LOGPALETTE *pLogpal)
	{
		return E_NOTIMPL;
	}

	/*IViewObject*/
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Draw( 
		/* [in] */ DWORD dwDrawAspect,
		/* [in] */ LONG lindex,
		/* [unique][in] */ void *pvAspect,
		/* [unique][in] */ DVTARGETDEVICE *ptd,
		/* [in] */ HDC hdcTargetDev,
		/* [in] */ HDC hdcDraw,
		/* [in] */ LPCRECTL lprcBounds,
		/* [unique][in] */ LPCRECTL lprcWBounds,
		/* [in] */ BOOL ( STDMETHODCALLTYPE *pfnContinue )( 
						  ULONG_PTR dwContinue),
		/* [in] */ ULONG_PTR dwContinue)
	{
		struct contextpublic *pub = (struct contextpublic*)plug;
		int width, height;
		HBITMAP bmp = (HBITMAP)funcs->GetSplashBack(plug, hdcDraw, &width, &height);
		if (bmp)
		{
			HDC memDC;
			RECT irect;
			irect.left = lprcBounds->left;
			irect.right = lprcBounds->right;
			irect.top = lprcBounds->top;
			irect.bottom = lprcBounds->bottom;

			memDC = CreateCompatibleDC(hdcDraw);
			SelectObject(memDC, bmp);
			StretchBlt(hdcDraw, irect.left, irect.top, irect.right-irect.left,irect.bottom-irect.top, memDC, 0, 0, width, height, SRCCOPY);
			SelectObject(memDC, NULL);
			DeleteDC(memDC);
			funcs->ReleaseSplashBack(plug, bmp);
		}
		if (*pub->statusmessage)
		{
			SetBkMode(hdcDraw, TRANSPARENT);
			TextOutA(hdcDraw, 0, 0, pub->statusmessage, strlen(pub->statusmessage));
		}

		return S_OK;
	}
    
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetColorSet( 
		/* [in] */ DWORD dwDrawAspect,
		/* [in] */ LONG lindex,
		/* [unique][in] */ void *pvAspect,
		/* [unique][in] */ DVTARGETDEVICE *ptd,
		/* [in] */ HDC hicTargetDev,
		/* [out] */ LOGPALETTE **ppColorSet)
	{
		return E_NOTIMPL;
	}
    
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Freeze( 
		/* [in] */ DWORD dwDrawAspect,
		/* [in] */ LONG lindex,
		/* [unique][in] */ void *pvAspect,
		/* [out] */ DWORD *pdwFreeze)
	{
		return E_NOTIMPL;
	}
    
	virtual HRESULT STDMETHODCALLTYPE Unfreeze( 
		/* [in] */ DWORD dwFreeze)
	{
		return E_NOTIMPL;
	}
    
	virtual HRESULT STDMETHODCALLTYPE SetAdvise( 
		/* [in] */ DWORD aspects,
		/* [in] */ DWORD advf,
		/* [unique][in] */ IAdviseSink *pAdvSink)
	{
		return E_NOTIMPL;
	}
    
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetAdvise( 
		/* [unique][out] */ DWORD *pAspects,
		/* [unique][out] */ DWORD *pAdvf,
		/* [out] */ IAdviseSink **ppAdvSink)
	{
		return E_NOTIMPL;
	}

	/*IOleInPlaceObjectWindowless*/
	virtual HRESULT STDMETHODCALLTYPE OnWindowMessage( 
		/* [in] */ UINT msg,
		/* [in] */ WPARAM wParam,
		/* [in] */ LPARAM lParam,
		/* [out] */ LRESULT *plResult)
	{
		switch(msg)
		{
		case WM_LBUTTONDOWN:
			funcs->SetInteger(plug, funcs->FindProp(plug, "running"), 1);
			return S_OK;
		default:
			return E_NOTIMPL;
		}
	}

	virtual HRESULT STDMETHODCALLTYPE GetDropTarget( 
		/* [out] */ IDropTarget **ppDropTarget)
	{
		return E_NOTIMPL;
	}

	/*IPersist*/
	virtual HRESULT STDMETHODCALLTYPE GetClassID( 
		/* [out] */ CLSID *pClassID)
	{
		return E_NOTIMPL;
	}

	/*IPersistPropertyBag2*/
	virtual HRESULT STDMETHODCALLTYPE InitNew( void)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Load( 
		/* [in] */ IPropertyBag2 *pPropBag,
		/* [in] */ IErrorLog *pErrLog)
	{
		PROPBAG2 prop[] =
		{
			{PROPBAG2_TYPE_DATA, VT_BSTR, 0, 0, (WCHAR *)L"splash", {0}},
			{PROPBAG2_TYPE_DATA, VT_BSTR, 0, 0, (WCHAR *)L"game", {0}},
			{PROPBAG2_TYPE_DATA, VT_BSTR, 0, 0, (WCHAR *)L"dataDownload", {0}}
		};
		VARIANT val[sizeof(prop)/sizeof(prop[0])];
		HRESULT res[sizeof(prop)/sizeof(prop[0])];
		memset(val, 0, sizeof(val));
		pPropBag->Read(sizeof(prop)/sizeof(prop[0]), prop, NULL, val, res);

		funcs->SetWString(plug, funcs->FindProp(plug, "splash"),		val[0].bstrVal);
		funcs->SetWString(plug, funcs->FindProp(plug, "game"),			val[1].bstrVal);
		funcs->SetWString(plug, funcs->FindProp(plug, "dataDownload"),	val[2].bstrVal);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Save( 
		/* [in] */ IPropertyBag2 *pPropBag,
		/* [in] */ BOOL fClearDirty,
		/* [in] */ BOOL fSaveAllProperties)
	{
		/*we don't actually save anything*/
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE IsDirty( void)
	{
		return E_NOTIMPL;
	}
};

const struct browserfuncs axfte::axbrowserfuncs = {NULL, axfte::statuschanged};


extern "C"
{

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	*ppv = NULL;

	if (rclsid == axfte_iid)
	{
		HRESULT res;
		axfte *newaxfte = new axfte();
		res = newaxfte->QueryInterface(riid, ppv);
		if (!*ppv)
			delete newaxfte;
		return res;
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
	return S_OK;
}

struct
{
	const char *key;
	const char *value;
} regkeys[] = 
{
	{"Software\\Classes\\FTE.FTEPlug\\",											"FTEPlug Class"},
	{"Software\\Classes\\FTE.FTEPlug\\CurVer\\",									"FTE.FTEPlug.1"},
	{"Software\\Classes\\FTE.FTEPlug.1\\",											"FTEPlug Class"},
	{"Software\\Classes\\FTE.FTEPlug.1\\CLSID\\",									"{"axfte_iid_str"}"},

	{"Software\\Classes\\CLSID\\{"axfte_iid_str"}\\",								""},
	{"Software\\Classes\\CLSID\\{"axfte_iid_str"}\\InprocHandler32\\",				"ole32.dll"},
	{"Software\\Classes\\CLSID\\{"axfte_iid_str"}\\InprocServer32\\",				"***DLLNAME***"},
	{"Software\\Classes\\CLSID\\{"axfte_iid_str"}\\InprocServer32\\ThreadingModel",	"Apartment"},
	{"Software\\Classes\\CLSID\\{"axfte_iid_str"}\\Programmable\\",					""},
	{"Software\\Classes\\CLSID\\{"axfte_iid_str"}\\VersionIndependentProgID\\",		"FTE.FTEPlug"},
	{"Software\\Classes\\CLSID\\{"axfte_iid_str"}\\ProgID\\",						"FTE.FTEPlug.1.0"},

	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\Description",										ENGINEWEBSITE},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\GeckoVersion",										"1.00"},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\Path",												"***DLLNAME***"},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\ProductName",										FULLENGINENAME},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\Vendor",											DISTRIBUTIONLONG},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\Version",											"***VERSION***"},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\MimeTypes\\application/x-fteplugin\\Description",	"FTE Game Engine Plugin"},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\MimeTypes\\application/x-ftemanifest\\Description",	"FTE Game File Manifest Listing"},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\MimeTypes\\application/x-qtv\\Description",			"QuakeTV Stream Information File"},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\MimeTypes\\application/x-qtv\\Suffixes",			"qtv"},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\Suffixes\\qtv",										""},
	{"Software\\MozillaPlugins\\@fteqw.com/FTE\\Suffixes\\mvd",										""},
	{NULL}
};
HRESULT WINAPI DllRegisterServer(void)
{
	char binaryname[1024];
	char tmp[1024];
	GetModuleFileName(global_hInstance, binaryname, sizeof(binaryname));

	HKEY h;
	bool allusers = false;
	int i;
	const char *ls;

	for (i = 0; regkeys[i].key; i++)
	{
		ls = strrchr(regkeys[i].key, '\\') + 1;
		memcpy(tmp, regkeys[i].key, ls - regkeys[i].key);
		tmp[ls - regkeys[i].key] = 0;

		if (RegCreateKeyExA(allusers?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER, tmp, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &h, NULL))
			continue;
		if (!strcmp(regkeys[i].value, "***DLLNAME***"))
			RegSetValueExA(h, ls, 0, REG_SZ, (BYTE*)binaryname, strlen(binaryname));
		else if (!strcmp(regkeys[i].value, "***VERSION***"))
		{
			char s[128];
#ifdef OFFICIAL_RELEASE
			Q_snprintfz(s, sizeof(s), "%s v%i.%02i", DISTRIBUTION, FTE_VER_MAJOR, FTE_VER_MINOR);
#elif defined(SVNREVISION)
			Q_snprintfz(s, sizeof(s), "%s SVN %s", DISTRIBUTION, STRINGIFY(SVNREVISION));
#else
			Q_snprintfz(s, sizeof(s), "%s build %s", DISTRIBUTION, __DATE__);
#endif
			RegSetValueExA(h, ls, 0, REG_SZ, (BYTE*)s, strlen(s));
		}
		else
			RegSetValueExA(h, ls, 0, REG_SZ, (BYTE*)regkeys[i].value, strlen(regkeys[i].value));
		RegCloseKey(h);
	}

	return S_OK;
}

HRESULT WINAPI DllUnregisterServer(void)
{
	int i;
	bool allusers = false;
	const char *ls;
	char tmp[1024];
	HKEY h;

	for (i = 0; regkeys[i].key; i++)
	{
	}

	/*go backwards*/
	for (i--; i>=0; i--)
	{
		ls = strrchr(regkeys[i].key, '\\') + 1;
		memcpy(tmp, regkeys[i].key, ls - regkeys[i].key);
		tmp[ls - regkeys[i].key] = 0;

		if (*ls)
		{
			h = NULL;
			if (!RegOpenKeyEx(allusers?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER, tmp, 0, KEY_SET_VALUE, &h))
			{
				RegDeleteValue(h, ls);
				RegCloseKey(h);
			}
		}
		else
			RegDeleteKey(allusers?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER, tmp);
	}

	return S_OK;
}

}//externC
#endif
