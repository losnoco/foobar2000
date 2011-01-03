

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0366 */
/* at Sun Jul 17 06:32:26 2005
 */
/* Compiler settings for .\foo_goggalor.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __foo_goggalor_h__
#define __foo_goggalor_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IfoobarCrawlPlugin_FWD_DEFINED__
#define __IfoobarCrawlPlugin_FWD_DEFINED__
typedef interface IfoobarCrawlPlugin IfoobarCrawlPlugin;
#endif 	/* __IfoobarCrawlPlugin_FWD_DEFINED__ */


#ifndef __foobarCrawlPlugin_FWD_DEFINED__
#define __foobarCrawlPlugin_FWD_DEFINED__

#ifdef __cplusplus
typedef class foobarCrawlPlugin foobarCrawlPlugin;
#else
typedef struct foobarCrawlPlugin foobarCrawlPlugin;
#endif /* __cplusplus */

#endif 	/* __foobarCrawlPlugin_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IfoobarCrawlPlugin_INTERFACE_DEFINED__
#define __IfoobarCrawlPlugin_INTERFACE_DEFINED__

/* interface IfoobarCrawlPlugin */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IfoobarCrawlPlugin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3B6BD9E6-6AD8-41B6-AB23-3F66243A6A7E")
    IfoobarCrawlPlugin : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HandleFile( 
            BSTR path,
            IDispatch *event_factory) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IfoobarCrawlPluginVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IfoobarCrawlPlugin * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IfoobarCrawlPlugin * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IfoobarCrawlPlugin * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IfoobarCrawlPlugin * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IfoobarCrawlPlugin * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IfoobarCrawlPlugin * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IfoobarCrawlPlugin * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HandleFile )( 
            IfoobarCrawlPlugin * This,
            BSTR path,
            IDispatch *event_factory);
        
        END_INTERFACE
    } IfoobarCrawlPluginVtbl;

    interface IfoobarCrawlPlugin
    {
        CONST_VTBL struct IfoobarCrawlPluginVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IfoobarCrawlPlugin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IfoobarCrawlPlugin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IfoobarCrawlPlugin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IfoobarCrawlPlugin_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IfoobarCrawlPlugin_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IfoobarCrawlPlugin_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IfoobarCrawlPlugin_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IfoobarCrawlPlugin_HandleFile(This,path,event_factory)	\
    (This)->lpVtbl -> HandleFile(This,path,event_factory)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IfoobarCrawlPlugin_HandleFile_Proxy( 
    IfoobarCrawlPlugin * This,
    BSTR path,
    IDispatch *event_factory);


void __RPC_STUB IfoobarCrawlPlugin_HandleFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IfoobarCrawlPlugin_INTERFACE_DEFINED__ */



#ifndef __foo_goggalorLib_LIBRARY_DEFINED__
#define __foo_goggalorLib_LIBRARY_DEFINED__

/* library foo_goggalorLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_foo_goggalorLib;

EXTERN_C const CLSID CLSID_foobarCrawlPlugin;

#ifdef __cplusplus

class DECLSPEC_UUID("FFB19A40-7A7D-44B2-B429-DA65D3ADB3D4")
foobarCrawlPlugin;
#endif
#endif /* __foo_goggalorLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


