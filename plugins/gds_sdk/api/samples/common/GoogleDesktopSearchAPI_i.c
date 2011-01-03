

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IGoogleDesktopSearchComponentRegistration,0x151857B2,0x26E0,0x4f4d,0xAC,0xED,0x4F,0x7E,0x4B,0x20,0x65,0xEF);


MIDL_DEFINE_GUID(IID, IID_IGoogleDesktopSearchComponentRegister,0x9B311E80,0xBC95,0x4518,0xA5,0x8C,0x44,0x6E,0xC9,0xA0,0x82,0xB5);


MIDL_DEFINE_GUID(IID, IID_IGoogleDesktopSearchEvent,0xBDAC0047,0x4759,0x43a1,0xBA,0x04,0xB1,0x48,0xE1,0x67,0x9E,0x87);


MIDL_DEFINE_GUID(IID, IID_IGoogleDesktopSearchEventFactory,0xA79E51C6,0xDB2D,0x4a44,0x84,0x8E,0xA8,0xEB,0xB2,0x2E,0x53,0x37);


MIDL_DEFINE_GUID(IID, DIID_DGoogleDesktopSearchFileNotify,0xA1DE6DB8,0xB20F,0x445c,0xBF,0xDE,0x16,0xC8,0xD5,0x3A,0x2F,0xA1);


MIDL_DEFINE_GUID(IID, LIBID_GoogleDesktopSearchAPILib,0x3D056FE7,0xEA8E,0x481a,0xB1,0x8F,0x0B,0x02,0xEB,0xF6,0xB3,0xC1);


MIDL_DEFINE_GUID(CLSID, CLSID_GoogleDesktopSearch,0x579822B3,0x44CD,0x4786,0x83,0xE0,0xAE,0x32,0xBC,0xB9,0xE6,0xB1);


MIDL_DEFINE_GUID(CLSID, CLSID_GoogleDesktopSearchRegister,0xBB8B07A0,0xB8D1,0x44e0,0xA2,0x62,0xC9,0xB7,0x21,0x2A,0xEC,0x68);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



