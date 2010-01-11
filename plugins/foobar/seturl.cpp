#include "pfc/pfc.h"
#include <utf8api.h>

// --------------------------------------------------------------------------------
// SetDlgItemUrl(hwnd,IDC_MYSTATIC,"http://www.wischik.com/lu");
//   This routine turns a dialog's static text control into an underlined hyperlink.
//   You can call it in your WM_INITDIALOG, or anywhere.
//   It will also set the text of the control... if you want to change the text
//   back, you can just call SetDlgItemText() afterwards.
// --------------------------------------------------------------------------------
void SetDlgItemUrl(HWND hdlg,int id,const char *url); 

// Implementation notes:
// We have to subclass both the static control (to set its cursor, to respond to click)
// and the dialog procedure (to set the font of the static control). Here are the two
// subclasses:
LRESULT CALLBACK UrlCtlProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK UrlDlgProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
// When the user calls SetDlgItemUrl, then the static-control-subclass is added
// if it wasn't already there, and the dialog-subclass is added if it wasn't
// already there. Both subclasses are removed in response to their respective
// WM_DESTROY messages. Also, each subclass stores a property in its window,
// which is a HGLOBAL handle to a GlobalAlloc'd structure:
typedef struct {char *url; WNDPROC oldproc; HFONT hf; HBRUSH hb;} TUrlData;
// I'm a miser and only defined a single structure, which is used by both
// the control-subclass and the dialog-subclass. Both of them use 'oldproc' of course.
// The control-subclass only uses 'url' (in response to WM_LBUTTONDOWN),
// and the dialog-subclass only uses 'hf' and 'hb' (in response to WM_CTLCOLORSTATIC)
// There is one sneaky thing to note. We create our underlined font *lazily*.
// Initially, hf is just NULL. But the first time the subclassed dialog received
// WM_CTLCOLORSTATIC, we sneak a peak at the font that was going to be used for
// the control, and we create our own copy of it but including the underline style.
// This way our code works fine on dialogs of any font.

// SetDlgItemUrl: this is the routine that sets up the subclassing.
void SetDlgItemUrl(HWND hdlg,int id,const char *url) 
{ // nb. vc7 has crummy warnings about 32/64bit. My code's perfect! That's why I hide the warnings.
  #pragma warning( push )
  #pragma warning( disable: 4312 4244 )
  // First we'll subclass the edit control
  HWND hctl = GetDlgItem(hdlg,id);
  //uSetWindowText(hctl,url);
  HGLOBAL hold = (HGLOBAL)GetProp(hctl,_T("href_dat"));
  if (hold!=NULL) // if it had been subclassed before, we merely need to tell it the new url
  { TUrlData *ud = (TUrlData*)GlobalLock(hold);
    delete[] ud->url;
    ud->url=new char[strlen(url)+1]; strcpy(ud->url,url);
  }
  else
  { HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE,sizeof(TUrlData));
    TUrlData *ud = (TUrlData*)GlobalLock(hglob);
    ud->oldproc = (WNDPROC)uGetWindowLong(hctl,GWL_WNDPROC);
    ud->url=new char[strlen(url)+1]; strcpy(ud->url,url);
    ud->hf=0; ud->hb=0;
    GlobalUnlock(hglob);
    SetProp(hctl,_T("href_dat"),hglob);
	uSetWindowLong(hctl, GWL_WNDPROC, (long)UrlCtlProc);
  }
  //
  // Second we subclass the dialog
  hold = (HGLOBAL)GetProp(hdlg,_T("href_dlg"));
  if (hold==NULL)
  { HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE,sizeof(TUrlData));
    TUrlData *ud = (TUrlData*)GlobalLock(hglob);
    ud->url=0;
    ud->oldproc = (WNDPROC)uGetWindowLong(hdlg,GWL_WNDPROC);
    ud->hb=CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    ud->hf=0; // the font will be created lazilly, the first time WM_CTLCOLORSTATIC gets called
    GlobalUnlock(hglob);
    SetProp(hdlg,_T("href_dlg"),hglob);
    uSetWindowLong(hdlg,GWL_WNDPROC,(LONG)UrlDlgProc);
  }
  #pragma warning( pop )
}

// UrlCtlProc: this is the subclass procedure for the static control
LRESULT CALLBACK UrlCtlProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{ HGLOBAL hglob = (HGLOBAL)GetProp(hwnd,_T("href_dat"));
  if (hglob==NULL) return uDefWindowProc(hwnd,msg,wParam,lParam);
  TUrlData *oud=(TUrlData*)GlobalLock(hglob); TUrlData ud=*oud;
  GlobalUnlock(hglob); // I made a copy of the structure just so I could GlobalUnlock it now, to be more local in my code
  switch (msg)
  { case WM_DESTROY:
    { RemoveProp(hwnd,_T("href_dat")); GlobalFree(hglob);
      if (ud.url!=0) delete[] ud.url;
      // nb. remember that ud.url is just a pointer to a memory block. It might look weird
      // for us to delete ud.url instead of oud->url, but they're both equivalent.
    } break;
    case WM_LBUTTONUP:
    { HWND hdlg=GetParent(hwnd); if (hdlg==0) hdlg=hwnd;
      //uShellExecute(hdlg,"open",ud.url,NULL,NULL,SW_SHOWNORMAL);
	  char * ptr = strchr( ud.url, ':' );
	  if ( ptr )
	  {
        string8_fastalloc key;
        key.set_string( ud.url, ptr - ud.url );
        key += "\\shell\\open\\command";

        HKEY hKey;

        BOOL isunicode = IsUnicode();

        if (isunicode)
        {
          if ( RegOpenKeyExW( HKEY_CLASSES_ROOT, string_utf16_from_utf8( key ), 0, KEY_READ, &hKey ) == ERROR_SUCCESS )
          {
            DWORD type;
            DWORD cbSize = 0;

            key.reset();

            if ( RegQueryValueExW( hKey, NULL, NULL, &type, NULL, &cbSize ) == ERROR_SUCCESS && type == REG_SZ )
            {
              mem_block_t<BYTE> data;
              data.set_size( cbSize + 1 );

              if ( RegQueryValueExW( hKey, NULL, NULL, &type, data.get_ptr(), &cbSize) == ERROR_SUCCESS && cbSize > 0 )
              {
                data[ cbSize ] = 0;
                key = string_utf8_from_utf16( reinterpret_cast<wchar_t *>( data.get_ptr() ) );
              }
            }

            RegCloseKey( hKey );
          }
          else key.reset();
        }
        else
        {
          if ( RegOpenKeyExA( HKEY_CLASSES_ROOT, string_ansi_from_utf8( key ), 0, KEY_READ, &hKey ) == ERROR_SUCCESS )
          {
            DWORD type;
            DWORD cbSize = 0;

            key.reset();

            if ( RegQueryValueExA( hKey, NULL, NULL, &type, NULL, &cbSize ) == ERROR_SUCCESS && type == REG_SZ )
            {
              mem_block_t<BYTE> data;
              data.set_size( cbSize + 1 );

              if ( RegQueryValueExA( hKey, NULL, NULL, &type, data.get_ptr(), &cbSize ) == ERROR_SUCCESS )
              {
                data[ cbSize ] = 0;
                key = string_utf8_from_ansi( reinterpret_cast<char *>( data.get_ptr() ) );
              }
            }

            RegCloseKey( hKey );
          }
          else key.reset();
        }

        if ( key.length() )
        {
          string8 command;

          int ptr2 = key.find_first( "%1" );

          if ( ptr2 >= 0 )
          {
            while ( ptr2 >= 0 && key[ptr2 + 2] != 0 && key[ptr2 + 2] != ' ' && !( ptr2 > 0 && key[ptr2 + 2] == '"' && key[ptr2 - 1] == '"' ) )
            {
              ptr2 = key.find_first( "%1", ptr2 + 2 );
            }

            if ( ptr2 >= 0 )
            {
              command.set_string( key, ptr2 );
              command += ud.url;
              command += key + ptr2 + 2;
            }
          }

          if ( ! command.length() )
          {
            command = key;
            command.add_byte(' ');

            if ( strchr( ud.url, ' ' ) )
            {
              command.add_byte('"');
              command += ud.url;
              command.add_byte('"');
            }
            else command += ud.url;
          }

          if (isunicode)
          {
            STARTUPINFOW startupInfo;
            PROCESS_INFORMATION procInfo;

            GetStartupInfoW( &startupInfo );
            CreateProcessW( NULL, ( WCHAR * )( string_utf16_from_utf8( command ).get_ptr() ), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &startupInfo, &procInfo );
          }
          else
          {
            STARTUPINFOA startupInfo;
            PROCESS_INFORMATION procInfo;

            GetStartupInfoA( &startupInfo );
            CreateProcessA( NULL, ( char * )( string_ansi_from_utf8( command ).get_ptr() ), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &startupInfo, &procInfo );
          }
        }
	  }
    } break;
    case WM_SETCURSOR:
    {
	  HCURSOR hc = LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND));
	  if (!hc) hc = LoadCursor(NULL, MAKEINTRESOURCE(IDC_UPARROW));
	  SetCursor(hc);
      return TRUE;
    } 
    case WM_NCHITTEST:
    { return HTCLIENT; // because normally a static returns HTTRANSPARENT, so disabling WM_SETCURSOR
    } 
  }
  return uCallWindowProc(ud.oldproc,hwnd,msg,wParam,lParam);
}
  
// UrlDlgProc: this is the subclass procedure for the dialog
LRESULT CALLBACK UrlDlgProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{ HGLOBAL hglob = (HGLOBAL)GetProp(hwnd,_T("href_dlg"));
  if (hglob==NULL) return uDefWindowProc(hwnd,msg,wParam,lParam);
  TUrlData *oud=(TUrlData*)GlobalLock(hglob); TUrlData ud=*oud;
  GlobalUnlock(hglob);
  switch (msg)
  { case WM_DESTROY:
    { RemoveProp(hwnd,_T("href_dlg")); GlobalFree(hglob);
      if (ud.hb!=0) DeleteObject(ud.hb);
      if (ud.hf!=0) DeleteObject(ud.hf);
    } break;
    case WM_CTLCOLORSTATIC:
    { HDC hdc=(HDC)wParam; HWND hctl=(HWND)lParam;
      // To check whether to handle this control, we look for its subclassed property!
      HANDLE hprop=GetProp(hctl,_T("href_dat"));
      if (hprop==NULL) return uCallWindowProc(ud.oldproc,hwnd,msg,wParam,lParam);
      // There has been a lot of faulty discussion in the newsgroups about how to change
      // the text colour of a static control. Lots of people mess around with the
      // TRANSPARENT text mode. That is incorrect. The correct solution is here:
      // (1) Leave the text opaque. This will allow us to re-SetDlgItemText without it looking wrong.
      // (2) SetBkColor. This background colour will be used underneath each character cell.
      // (3) return HBRUSH. This background colour will be used where there's no text.
      SetTextColor(hdc,RGB(0,0,255));
      SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));
      if (ud.hf==0)
      { // we use lazy creation of the font. That's so we can see font was currently being used.
        LOGFONT lf;
        lf.lfWidth=0;
        lf.lfEscapement=0;
        lf.lfOrientation=0;
        lf.lfUnderline=TRUE;
        lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
        lf.lfQuality=DEFAULT_QUALITY;

		TEXTMETRIC tm; GetTextMetrics(hdc,&tm);
		lf.lfHeight=tm.tmHeight;
		lf.lfWeight=tm.tmWeight;
		lf.lfItalic=tm.tmItalic;
		lf.lfStrikeOut=tm.tmStruckOut;
		lf.lfCharSet=tm.tmCharSet;
		lf.lfPitchAndFamily=tm.tmPitchAndFamily;

		GetTextFace(hdc,LF_FACESIZE,lf.lfFaceName);

		ud.hf=CreateFontIndirect(&lf);
        TUrlData *oud = (TUrlData*)GlobalLock(hglob); oud->hf=ud.hf; GlobalUnlock(hglob);
      }
      SelectObject(hdc,ud.hf);
      // Note: the win32 docs say to return an HBRUSH, typecast as a BOOL. But they
      // fail to explain how this will work in 64bit windows where an HBRUSH is 64bit.
      // I have supressed the warnings for now, because I hate them...
      #pragma warning( push )
      #pragma warning( disable: 4311 )
      return (BOOL)ud.hb;
      #pragma warning( pop )
    }  
  }
  return uCallWindowProc(ud.oldproc,hwnd,msg,wParam,lParam);
}