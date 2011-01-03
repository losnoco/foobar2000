

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0366 */
/* at Sun Jul 17 06:34:19 2005
 */
/* Compiler settings for .\goggalor_proxy.idl:
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

#ifndef __goggalor_proxy_h__
#define __goggalor_proxy_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IfoobarCrawlProxy_FWD_DEFINED__
#define __IfoobarCrawlProxy_FWD_DEFINED__
typedef interface IfoobarCrawlProxy IfoobarCrawlProxy;
#endif 	/* __IfoobarCrawlProxy_FWD_DEFINED__ */


#ifndef __foobarCrawlProxy_FWD_DEFINED__
#define __foobarCrawlProxy_FWD_DEFINED__

#ifdef __cplusplus
typedef class foobarCrawlProxy foobarCrawlProxy;
#else
typedef struct foobarCrawlProxy foobarCrawlProxy;
#endif /* __cplusplus */

#endif 	/* __foobarCrawlProxy_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IfoobarCrawlProxy_INTERFACE_DEFINED__
#define __IfoobarCrawlProxy_INTERFACE_DEFINED__

/* interface IfoobarCrawlProxy */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IfoobarCrawlProxy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4CAC5844-45EE-4E8D-B7E7-E2C91E76E5DE")
    IfoobarCrawlProxy : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HandleFile( 
            BSTR path,
            IDispatch *event_factory) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IfoobarCrawlProxyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IfoobarCrawlProxy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IfoobarCrawlProxy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IfoobarCrawlProxy * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IfoobarCrawlProxy * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IfoobarCrawlProxy * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IfoobarCrawlProxy * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IfoobarCrawlProxy * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HandleFile )( 
            IfoobarCrawlProxy * This,
            BSTR path,
            IDispatch *event_factory);
        
        END_INTERFACE
    } IfoobarCrawlProxyVtbl;

    interface IfoobarCrawlProxy
    {
        CONST_VTBL struct IfoobarCrawlProxyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IfoobarCrawlProxy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IfoobarCrawlProxy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IfoobarCrawlProxy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IfoobarCrawlProxy_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IfoobarCrawlProxy_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IfoobarCrawlProxy_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IfoobarCrawlProxy_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IfoobarCrawlProxy_HandleFile(This,path,event_factory)	\
    (This)->lpVtbl -> HandleFile(This,path,event_factory)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IfoobarCrawlProxy_HandleFile_Proxy( 
    IfoobarCrawlProxy * This,
    BSTR path,
    IDispatch *event_factory);


void __RPC_STUB IfoobarCrawlProxy_HandleFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IfoobarCrawlProxy_INTERFACE_DEFINED__ */



#ifndef __goggalor_proxyLib_LIBRARY_DEFINED__
#define __goggalor_proxyLib_LIBRARY_DEFINED__

/* library goggalor_proxyLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_goggalor_proxyLib;

EXTERN_C const CLSID CLSID_foobarCrawlProxy;

#ifdef __cplusplus

class DECLSPEC_UUID("D762E411-6F39-4752-8814-CCB40675A73C")
foobarCrawlProxy;
#endif
#endif /* __goggalor_proxyLib_LIBRARY_DEFINED__ */

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


