

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0366 */
/* at Fri Jul 15 04:11:57 2005
 */
/* Compiler settings for .\GoogleDesktopSearchAPI.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __GoogleDesktopSearchAPI_h__
#define __GoogleDesktopSearchAPI_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IGoogleDesktopSearchComponentRegistration_FWD_DEFINED__
#define __IGoogleDesktopSearchComponentRegistration_FWD_DEFINED__
typedef interface IGoogleDesktopSearchComponentRegistration IGoogleDesktopSearchComponentRegistration;
#endif 	/* __IGoogleDesktopSearchComponentRegistration_FWD_DEFINED__ */


#ifndef __IGoogleDesktopSearchComponentRegister_FWD_DEFINED__
#define __IGoogleDesktopSearchComponentRegister_FWD_DEFINED__
typedef interface IGoogleDesktopSearchComponentRegister IGoogleDesktopSearchComponentRegister;
#endif 	/* __IGoogleDesktopSearchComponentRegister_FWD_DEFINED__ */


#ifndef __IGoogleDesktopSearchEvent_FWD_DEFINED__
#define __IGoogleDesktopSearchEvent_FWD_DEFINED__
typedef interface IGoogleDesktopSearchEvent IGoogleDesktopSearchEvent;
#endif 	/* __IGoogleDesktopSearchEvent_FWD_DEFINED__ */


#ifndef __IGoogleDesktopSearchEventFactory_FWD_DEFINED__
#define __IGoogleDesktopSearchEventFactory_FWD_DEFINED__
typedef interface IGoogleDesktopSearchEventFactory IGoogleDesktopSearchEventFactory;
#endif 	/* __IGoogleDesktopSearchEventFactory_FWD_DEFINED__ */


#ifndef __DGoogleDesktopSearchFileNotify_FWD_DEFINED__
#define __DGoogleDesktopSearchFileNotify_FWD_DEFINED__
typedef interface DGoogleDesktopSearchFileNotify DGoogleDesktopSearchFileNotify;
#endif 	/* __DGoogleDesktopSearchFileNotify_FWD_DEFINED__ */


#ifndef __DGoogleDesktopSearchFileNotify_FWD_DEFINED__
#define __DGoogleDesktopSearchFileNotify_FWD_DEFINED__
typedef interface DGoogleDesktopSearchFileNotify DGoogleDesktopSearchFileNotify;
#endif 	/* __DGoogleDesktopSearchFileNotify_FWD_DEFINED__ */


#ifndef __IGoogleDesktopSearchEvent_FWD_DEFINED__
#define __IGoogleDesktopSearchEvent_FWD_DEFINED__
typedef interface IGoogleDesktopSearchEvent IGoogleDesktopSearchEvent;
#endif 	/* __IGoogleDesktopSearchEvent_FWD_DEFINED__ */


#ifndef __GoogleDesktopSearch_FWD_DEFINED__
#define __GoogleDesktopSearch_FWD_DEFINED__

#ifdef __cplusplus
typedef class GoogleDesktopSearch GoogleDesktopSearch;
#else
typedef struct GoogleDesktopSearch GoogleDesktopSearch;
#endif /* __cplusplus */

#endif 	/* __GoogleDesktopSearch_FWD_DEFINED__ */


#ifndef __GoogleDesktopSearchRegister_FWD_DEFINED__
#define __GoogleDesktopSearchRegister_FWD_DEFINED__

#ifdef __cplusplus
typedef class GoogleDesktopSearchRegister GoogleDesktopSearchRegister;
#else
typedef struct GoogleDesktopSearchRegister GoogleDesktopSearchRegister;
#endif /* __cplusplus */

#endif 	/* __GoogleDesktopSearchRegister_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_GoogleDesktopSearchAPI_0000 */
/* [local] */ 

#include <winerror.h>
#define E_EXTENSION_REGISTERED           MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0001)
#define E_COMPONENT_NOT_REGISTERED       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0002)
#define E_NO_SUCH_SCHEMA                 MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0003)
#define E_NO_SUCH_PROPERTY               MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0004)
#define E_COMPONENT_DISABLED             MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0005)
#define E_COMPONENT_ALREADY_REGISTERED   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0006)
#define S_INDEXING_PAUSED                MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x0007)
#define E_EVENT_TOO_LARGE                MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0008)
#define E_SERVICE_NOT_RUNNING            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0009)
#define E_INVALID_EVENT_FLAGS            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x000A)


extern RPC_IF_HANDLE __MIDL_itf_GoogleDesktopSearchAPI_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_GoogleDesktopSearchAPI_0000_v0_0_s_ifspec;

#ifndef __IGoogleDesktopSearchComponentRegistration_INTERFACE_DEFINED__
#define __IGoogleDesktopSearchComponentRegistration_INTERFACE_DEFINED__

/* interface IGoogleDesktopSearchComponentRegistration */
/* [unique][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IGoogleDesktopSearchComponentRegistration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("151857B2-26E0-4f4d-ACED-4F7E4B2065EF")
    IGoogleDesktopSearchComponentRegistration : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegisterExtension( 
            /* [in] */ BSTR extension_handled) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGoogleDesktopSearchComponentRegistrationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGoogleDesktopSearchComponentRegistration * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGoogleDesktopSearchComponentRegistration * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGoogleDesktopSearchComponentRegistration * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IGoogleDesktopSearchComponentRegistration * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IGoogleDesktopSearchComponentRegistration * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IGoogleDesktopSearchComponentRegistration * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IGoogleDesktopSearchComponentRegistration * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RegisterExtension )( 
            IGoogleDesktopSearchComponentRegistration * This,
            /* [in] */ BSTR extension_handled);
        
        END_INTERFACE
    } IGoogleDesktopSearchComponentRegistrationVtbl;

    interface IGoogleDesktopSearchComponentRegistration
    {
        CONST_VTBL struct IGoogleDesktopSearchComponentRegistrationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGoogleDesktopSearchComponentRegistration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGoogleDesktopSearchComponentRegistration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGoogleDesktopSearchComponentRegistration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGoogleDesktopSearchComponentRegistration_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IGoogleDesktopSearchComponentRegistration_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IGoogleDesktopSearchComponentRegistration_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IGoogleDesktopSearchComponentRegistration_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IGoogleDesktopSearchComponentRegistration_RegisterExtension(This,extension_handled)	\
    (This)->lpVtbl -> RegisterExtension(This,extension_handled)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IGoogleDesktopSearchComponentRegistration_RegisterExtension_Proxy( 
    IGoogleDesktopSearchComponentRegistration * This,
    /* [in] */ BSTR extension_handled);


void __RPC_STUB IGoogleDesktopSearchComponentRegistration_RegisterExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGoogleDesktopSearchComponentRegistration_INTERFACE_DEFINED__ */


#ifndef __IGoogleDesktopSearchComponentRegister_INTERFACE_DEFINED__
#define __IGoogleDesktopSearchComponentRegister_INTERFACE_DEFINED__

/* interface IGoogleDesktopSearchComponentRegister */
/* [unique][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IGoogleDesktopSearchComponentRegister;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9B311E80-BC95-4518-A58C-446EC9A082B5")
    IGoogleDesktopSearchComponentRegister : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegisterComponent( 
            /* [in] */ BSTR component_guid_or_progid,
            /* [in] */ VARIANT component_description,
            /* [retval][out] */ IGoogleDesktopSearchComponentRegistration **registration) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UnregisterComponent( 
            /* [in] */ BSTR component_guid_or_progid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGoogleDesktopSearchComponentRegisterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGoogleDesktopSearchComponentRegister * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGoogleDesktopSearchComponentRegister * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGoogleDesktopSearchComponentRegister * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IGoogleDesktopSearchComponentRegister * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IGoogleDesktopSearchComponentRegister * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IGoogleDesktopSearchComponentRegister * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IGoogleDesktopSearchComponentRegister * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RegisterComponent )( 
            IGoogleDesktopSearchComponentRegister * This,
            /* [in] */ BSTR component_guid_or_progid,
            /* [in] */ VARIANT component_description,
            /* [retval][out] */ IGoogleDesktopSearchComponentRegistration **registration);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UnregisterComponent )( 
            IGoogleDesktopSearchComponentRegister * This,
            /* [in] */ BSTR component_guid_or_progid);
        
        END_INTERFACE
    } IGoogleDesktopSearchComponentRegisterVtbl;

    interface IGoogleDesktopSearchComponentRegister
    {
        CONST_VTBL struct IGoogleDesktopSearchComponentRegisterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGoogleDesktopSearchComponentRegister_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGoogleDesktopSearchComponentRegister_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGoogleDesktopSearchComponentRegister_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGoogleDesktopSearchComponentRegister_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IGoogleDesktopSearchComponentRegister_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IGoogleDesktopSearchComponentRegister_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IGoogleDesktopSearchComponentRegister_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IGoogleDesktopSearchComponentRegister_RegisterComponent(This,component_guid_or_progid,component_description,registration)	\
    (This)->lpVtbl -> RegisterComponent(This,component_guid_or_progid,component_description,registration)

#define IGoogleDesktopSearchComponentRegister_UnregisterComponent(This,component_guid_or_progid)	\
    (This)->lpVtbl -> UnregisterComponent(This,component_guid_or_progid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IGoogleDesktopSearchComponentRegister_RegisterComponent_Proxy( 
    IGoogleDesktopSearchComponentRegister * This,
    /* [in] */ BSTR component_guid_or_progid,
    /* [in] */ VARIANT component_description,
    /* [retval][out] */ IGoogleDesktopSearchComponentRegistration **registration);


void __RPC_STUB IGoogleDesktopSearchComponentRegister_RegisterComponent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IGoogleDesktopSearchComponentRegister_UnregisterComponent_Proxy( 
    IGoogleDesktopSearchComponentRegister * This,
    /* [in] */ BSTR component_guid_or_progid);


void __RPC_STUB IGoogleDesktopSearchComponentRegister_UnregisterComponent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGoogleDesktopSearchComponentRegister_INTERFACE_DEFINED__ */


#ifndef __IGoogleDesktopSearchEvent_INTERFACE_DEFINED__
#define __IGoogleDesktopSearchEvent_INTERFACE_DEFINED__

/* interface IGoogleDesktopSearchEvent */
/* [unique][oleautomation][dual][uuid][object] */ 


enum EventFlags
    {	EventFlagIndexable	= 0x1,
	EventFlagHistorical	= 0x10
    } ;

EXTERN_C const IID IID_IGoogleDesktopSearchEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BDAC0047-4759-43a1-BA04-B148E1679E87")
    IGoogleDesktopSearchEvent : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddProperty( 
            BSTR property_name,
            VARIANT property_value) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Send( 
            long event_flags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGoogleDesktopSearchEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGoogleDesktopSearchEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGoogleDesktopSearchEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGoogleDesktopSearchEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IGoogleDesktopSearchEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IGoogleDesktopSearchEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IGoogleDesktopSearchEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IGoogleDesktopSearchEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddProperty )( 
            IGoogleDesktopSearchEvent * This,
            BSTR property_name,
            VARIANT property_value);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Send )( 
            IGoogleDesktopSearchEvent * This,
            long event_flags);
        
        END_INTERFACE
    } IGoogleDesktopSearchEventVtbl;

    interface IGoogleDesktopSearchEvent
    {
        CONST_VTBL struct IGoogleDesktopSearchEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGoogleDesktopSearchEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGoogleDesktopSearchEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGoogleDesktopSearchEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGoogleDesktopSearchEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IGoogleDesktopSearchEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IGoogleDesktopSearchEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IGoogleDesktopSearchEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IGoogleDesktopSearchEvent_AddProperty(This,property_name,property_value)	\
    (This)->lpVtbl -> AddProperty(This,property_name,property_value)

#define IGoogleDesktopSearchEvent_Send(This,event_flags)	\
    (This)->lpVtbl -> Send(This,event_flags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IGoogleDesktopSearchEvent_AddProperty_Proxy( 
    IGoogleDesktopSearchEvent * This,
    BSTR property_name,
    VARIANT property_value);


void __RPC_STUB IGoogleDesktopSearchEvent_AddProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IGoogleDesktopSearchEvent_Send_Proxy( 
    IGoogleDesktopSearchEvent * This,
    long event_flags);


void __RPC_STUB IGoogleDesktopSearchEvent_Send_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGoogleDesktopSearchEvent_INTERFACE_DEFINED__ */


#ifndef __IGoogleDesktopSearchEventFactory_INTERFACE_DEFINED__
#define __IGoogleDesktopSearchEventFactory_INTERFACE_DEFINED__

/* interface IGoogleDesktopSearchEventFactory */
/* [unique][oleautomation][dual][uuid][object] */ 


EXTERN_C const IID IID_IGoogleDesktopSearchEventFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A79E51C6-DB2D-4a44-848E-A8EBB22E5337")
    IGoogleDesktopSearchEventFactory : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateEvent( 
            /* [in] */ BSTR component_guid_or_progid,
            /* [in] */ BSTR schema_name,
            /* [retval][out] */ IDispatch **event) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGoogleDesktopSearchEventFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGoogleDesktopSearchEventFactory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGoogleDesktopSearchEventFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGoogleDesktopSearchEventFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IGoogleDesktopSearchEventFactory * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IGoogleDesktopSearchEventFactory * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IGoogleDesktopSearchEventFactory * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IGoogleDesktopSearchEventFactory * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateEvent )( 
            IGoogleDesktopSearchEventFactory * This,
            /* [in] */ BSTR component_guid_or_progid,
            /* [in] */ BSTR schema_name,
            /* [retval][out] */ IDispatch **event);
        
        END_INTERFACE
    } IGoogleDesktopSearchEventFactoryVtbl;

    interface IGoogleDesktopSearchEventFactory
    {
        CONST_VTBL struct IGoogleDesktopSearchEventFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGoogleDesktopSearchEventFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGoogleDesktopSearchEventFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGoogleDesktopSearchEventFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGoogleDesktopSearchEventFactory_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IGoogleDesktopSearchEventFactory_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IGoogleDesktopSearchEventFactory_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IGoogleDesktopSearchEventFactory_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IGoogleDesktopSearchEventFactory_CreateEvent(This,component_guid_or_progid,schema_name,event)	\
    (This)->lpVtbl -> CreateEvent(This,component_guid_or_progid,schema_name,event)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IGoogleDesktopSearchEventFactory_CreateEvent_Proxy( 
    IGoogleDesktopSearchEventFactory * This,
    /* [in] */ BSTR component_guid_or_progid,
    /* [in] */ BSTR schema_name,
    /* [retval][out] */ IDispatch **event);


void __RPC_STUB IGoogleDesktopSearchEventFactory_CreateEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGoogleDesktopSearchEventFactory_INTERFACE_DEFINED__ */



#ifndef __GoogleDesktopSearchAPILib_LIBRARY_DEFINED__
#define __GoogleDesktopSearchAPILib_LIBRARY_DEFINED__

/* library GoogleDesktopSearchAPILib */
/* [helpstring][version][uuid] */ 




EXTERN_C const IID LIBID_GoogleDesktopSearchAPILib;

#ifndef __DGoogleDesktopSearchFileNotify_DISPINTERFACE_DEFINED__
#define __DGoogleDesktopSearchFileNotify_DISPINTERFACE_DEFINED__

/* dispinterface DGoogleDesktopSearchFileNotify */
/* [uuid] */ 


EXTERN_C const IID DIID_DGoogleDesktopSearchFileNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("A1DE6DB8-B20F-445c-BFDE-16C8D53A2FA1")
    DGoogleDesktopSearchFileNotify : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DGoogleDesktopSearchFileNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DGoogleDesktopSearchFileNotify * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DGoogleDesktopSearchFileNotify * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DGoogleDesktopSearchFileNotify * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DGoogleDesktopSearchFileNotify * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DGoogleDesktopSearchFileNotify * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DGoogleDesktopSearchFileNotify * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DGoogleDesktopSearchFileNotify * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DGoogleDesktopSearchFileNotifyVtbl;

    interface DGoogleDesktopSearchFileNotify
    {
        CONST_VTBL struct DGoogleDesktopSearchFileNotifyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DGoogleDesktopSearchFileNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DGoogleDesktopSearchFileNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DGoogleDesktopSearchFileNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DGoogleDesktopSearchFileNotify_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DGoogleDesktopSearchFileNotify_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DGoogleDesktopSearchFileNotify_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DGoogleDesktopSearchFileNotify_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DGoogleDesktopSearchFileNotify_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_GoogleDesktopSearch;

#ifdef __cplusplus

class DECLSPEC_UUID("579822B3-44CD-4786-83E0-AE32BCB9E6B1")
GoogleDesktopSearch;
#endif

EXTERN_C const CLSID CLSID_GoogleDesktopSearchRegister;

#ifdef __cplusplus

class DECLSPEC_UUID("BB8B07A0-B8D1-44e0-A262-C9B7212AEC68")
GoogleDesktopSearchRegister;
#endif
#endif /* __GoogleDesktopSearchAPILib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


