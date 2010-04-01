#include <foobar2000.h>
#include "../ATLHelpers/ATLHelpers.h"
#include <atlframe.h>

#include <gme/Spc_Dsp.h>

#include <math.h>

static const char spc_vis_field_registers[]="spc_dsp_registers";
static const char spc_vis_field_env_modes[]="spc_dsp_env_modes";
static const char spc_vis_field_out_levels[]="spc_dsp_out_levels";

COLORREF ColorPreset[4][22] = 
{
	{RGB(0,0,0),RGB(255,255,255),RGB(0,128,0),RGB(0,255,0),RGB(0,128,0),RGB(0,255,0),RGB(0,128,0),RGB(0,255,0),RGB(0,128,0),RGB(0,255,0),RGB(0,128,0),RGB(0,255,0),RGB(0,128,0),RGB(0,255,0),RGB(0,128,0),RGB(0,255,0),RGB(0,255,0),RGB(255,0,0),RGB(0,255,0),RGB(255,0,0),RGB(0,255,0),RGB(255,0,0)},
	{RGB(0,0,0),RGB(255,128,128),RGB(230,115,0),RGB(255,182,108),RGB(255,0,255),RGB(255,113,255),RGB(0,164,164),RGB(102,255,255),RGB(164,53,0),RGB(255,152,102),RGB(0,0,164),RGB(102,102,255),RGB(164,0,164),RGB(255,102,255),RGB(164,164,0),RGB(255,255,102),RGB(16,0,0),RGB(255,0,0),RGB(0,16,0),RGB(0,255,0),RGB(0,0,16),RGB(0,0,255)},
	{RGB(64,128,128),RGB(255,255,255),RGB(0,121,0),RGB(0,255,0),RGB(0,0,174),RGB(128,128,255),RGB(121,61,0),RGB(255,128,0),RGB(132,132,0),RGB(255,255,0),RGB(117,0,117),RGB(255,0,255),RGB(0,0,132),RGB(0,0,255),RGB(121,0,0),RGB(255,0,0),RGB(100,0,100),RGB(255,255,255),RGB(0,0,100),RGB(255,255,255),RGB(0,100,0),RGB(255,255,255)},
	{RGB(64,69,77),RGB(192,192,192),RGB(128,128,128),RGB(255,255,255),RGB(128,128,128),RGB(255,255,255),RGB(128,128,128),RGB(255,255,255),RGB(128,128,128),RGB(255,255,255),RGB(128,128,128),RGB(255,255,255),RGB(128,128,128),RGB(255,255,255),RGB(128,128,128),RGB(255,255,255),RGB(128,128,128),RGB(255,255,255),RGB(128,128,128),RGB(255,255,255),RGB(128,128,128),RGB(255,255,255)}
};

// configuration for the popup window
/*static const GUID guid_cfg_labels = { 0x2fdb3f5e, 0x2df9, 0x4efe, { 0xb0, 0xa8, 0x7e, 0x29, 0x73, 0x79, 0xbf, 0xbc } };
static const GUID guid_cfg_style = { 0x45e05961, 0x6584, 0x4286, { 0x9a, 0x7f, 0x7c, 0x40, 0xa0, 0xa, 0x58, 0x95 } };
static const GUID guid_cfg_placement = { 0x600fba47, 0xea03, 0x40b1, { 0x9e, 0xb7, 0x34, 0xf2, 0xa1, 0x68, 0x9d, 0x6e } };

static cfg_bool cfg_labels( guid_cfg_labels, true );
static cfg_int cfg_style( guid_cfg_style, 0 );
static cfg_window_placement cfg_placement(guid_cfg_placement);*/

class CVisWindow : public CWindowImpl<CVisWindow>, play_callback
{
protected:
	enum EnvMode { Release = 0, Attack, Decay, Sustain, Dec, Exp, Inc, Bent, Direct };

	struct EnvM
	{
		unsigned char val;
		EnvM() { val = 0; }
		EnvM( EnvMode mode ) { val = mode; }
		EnvM( Spc_Dsp::env_mode_t env_mode, unsigned char adsr0, unsigned char gain ) { set( env_mode, adsr0, gain ); }
		unsigned set( Spc_Dsp::env_mode_t env_mode, unsigned char adsr0, unsigned char gain )
		{
			if ( adsr0 & 0x80 ) val = env_mode;
			else
			{
				unsigned mode = gain >> 5;
				if ( mode < 4 ) val = Direct;
				else
				{
					val = mode - 4 + Dec;
				}
			}
			return val;
		}
		operator const unsigned () const { return val; }
		unsigned operator= ( unsigned _val ) { return val = _val; }
		unsigned operator= ( const EnvM & env ) { return val = env; }
		unsigned operator= ( EnvMode mode ) { return val = mode; }
	};

	bool m_bVisRunning;

	bool m_bVisShowLabels;
	unsigned m_bVisColorStyle;

	unsigned char regs[Spc_Dsp::register_count];
	EnvM env_modes[Spc_Dsp::voice_count];
	int out_levels[Spc_Dsp::voice_count * 2];

	unsigned char old_regs[Spc_Dsp::register_count];
	EnvM old_env_modes[Spc_Dsp::voice_count];
	int old_out_levels[Spc_Dsp::voice_count * 2];

	HDC m_hDC;
	BOOL m_bDrawAll;
	struct{TCHAR szName[20];EnvM mode;COLORREF cr;}m_EnvTypes[9];
	struct{TCHAR szName[20];int x,y;}m_Columns[12];
	HPEN m_Pens[26];
	HBRUSH m_Brushes[22], m_BackBrush;
	COLORREF m_Colors[22];
	HBITMAP m_hBmpGroup[7];
	HDC m_hDCGroup[7];
	HFONT m_hFont,m_hOldFont;

public:
	CVisWindow()
	{
		m_bVisRunning = false;

		memset( old_regs, 0, sizeof(old_regs) );
		memset( old_env_modes, 0xFF, sizeof(old_env_modes) );
		memset( old_out_levels, 0, sizeof(old_out_levels) );

		LPCTSTR lpszColNames[]={_T("Channel"),_T("Voice"),_T("Key On"),_T("Echo"),_T("Pitch Mod."),_T("Noise"),_T("Surround"),_T("Env. Type"),_T("Envelope"),_T("Pitch"),_T("Volume"),_T("")};
		for(int i=0;i<tabsize(m_Columns);i++)
			_tcscpy(m_Columns[i].szName,lpszColNames[i]);

		struct{LPCTSTR lpszName;EnvM mode;COLORREF cr;}
		EnvTypes[]=
		{
			_T("LinDec"),Dec,RGB(0,170,200),
			_T("ExpDec"),Exp,RGB(255,128,0),
			_T("LinInc"),Inc,RGB(0,0,255),
			_T("BentInc"),Bent,RGB(255,255,0),
			_T("Release"),Release,RGB(255,0,255),
			_T("Sustain"),Sustain,RGB(0,255,255),
			_T("Attack"),Attack,RGB(255,0,0),
			_T("Decay"),Decay,RGB(64,128,128),
			_T("Direct"),Direct,RGB(255,187,119),
		};
		for(int i=0;i<sizeof(m_EnvTypes)/sizeof(m_EnvTypes[0]);i++)
		{
			_tcscpy(m_EnvTypes[i].szName,EnvTypes[i].lpszName);
			m_EnvTypes[i].mode=EnvTypes[i].mode;
			m_EnvTypes[i].cr=EnvTypes[i].cr;
		}
	}

	DECLARE_WND_CLASS_EX( _T("D087F204-63D3-4B24-AA7F-8E7C364616A2"), 0, -1 )

	BEGIN_MSG_MAP(CVisWindow)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_DESTROY(OnDestroy)
	END_MSG_MAP()

	// play_callback interface
	void FB2KAPI on_playback_starting(play_control::t_track_command,bool) {}
	void FB2KAPI on_playback_edited(metadb_handle_ptr) {}
	void FB2KAPI on_playback_seek(double) {}
	void FB2KAPI on_playback_pause(bool p_state) {}
	void FB2KAPI on_playback_dynamic_info_track(const file_info &) {}
	void FB2KAPI on_playback_time(double) {}
	void FB2KAPI on_volume_change(float) {}
	
	void FB2KAPI on_playback_new_track(metadb_handle_ptr p_track)
	{
		if ( p_track.is_valid() )
		{
			file_info_impl m_info;
			if ( p_track->get_info_async( m_info ) )
			{
				const char * p_codec = m_info.info_get( "codec" );
				if ( p_codec && !pfc::stricmp_ascii( p_codec, "SPC" ) )
				{
					m_bVisRunning = true;
					m_bDrawAll = TRUE;
					Invalidate();
					return;
				}
			}
		}
		m_bVisRunning = false;
		m_bDrawAll = TRUE;
		Invalidate();
	}

	void FB2KAPI on_playback_stop(play_control::t_stop_reason)
	{
		m_bVisRunning = false;
		m_bDrawAll = TRUE;
		Invalidate();
	}

	void FB2KAPI on_playback_dynamic_info(const file_info & p_info)
	{
		const char * p_codec = p_info.info_get( "codec" );
		if ( p_codec && !pfc::stricmp_ascii( p_codec, "SPC" ) )
		{
			if (!m_bVisRunning)
			{
				m_bVisRunning = true;
				m_bDrawAll = TRUE;
			}

			const char * p_registers = p_info.info_get( spc_vis_field_registers );
			if ( p_registers )
			{
				for ( unsigned i = 0, j = min( Spc_Dsp::register_count, strlen( p_registers ) / 2 ); i < j; i++ )
				{
					unsigned val = p_registers[ i * 2 ];
					if ( val <= '9' && val >= '0' ) val -= '0';
					else if ( val <= 'F' && val >= 'A') val -= 'A' - 10;
					else if ( val <= 'f' && val >= 'a' ) val -= 'a' - 10;
					else val = 0;
					unsigned val2 = p_registers[ i * 2 + 1 ];
					if ( val2 <= '9' && val2 >= '0' ) val2 -= '0';
					else if ( val2 <= 'F' && val2 >= 'A') val2 -= 'A' - 10;
					else if ( val2 <= 'f' && val2 >= 'a' ) val2 -= 'a' - 10;
					else val2 = 0;
					regs[ i ] = ( val << 4 ) | val2;
				}
			}

			const char * p_env_modes = p_info.info_get( spc_vis_field_env_modes );
			if ( p_env_modes )
			{
				for ( unsigned i = 0, j = min( Spc_Dsp::voice_count, strlen( p_env_modes ) ); i < j; i++ )
				{
					unsigned val = p_env_modes[ i ] - '0';
					Spc_Dsp::env_mode_t env_mode = Spc_Dsp::env_release;
					Spc_Dsp::env_mode_t env_modes[] = { Spc_Dsp::env_release, Spc_Dsp::env_attack, Spc_Dsp::env_decay, Spc_Dsp::env_sustain };
					if ( val < 4 ) env_mode = env_modes[ val ];
					this->env_modes[ i ].set( env_mode, regs[ 0x10 * i + Spc_Dsp::v_adsr0 ], regs[ 0x10 * i + Spc_Dsp::v_gain ] );
				}
			}

			const char * p_out_levels = p_info.info_get( spc_vis_field_out_levels );
			if ( p_out_levels )
			{
				for ( unsigned i = 0, j = min( Spc_Dsp::voice_count * 2, strlen( p_out_levels ) / 4 ); i < j; i++ )
				{
					pfc::string8 str( p_out_levels + i * 4, 4 );
					char * end;
					int val = strtol( str, &end, 16 );
					if ( *end == 0 ) out_levels[ i ] = val;
				}
			}

			Render();
		}
		else if (m_bVisRunning)
		{
			m_bVisRunning = false;
			m_bDrawAll = TRUE;
			Invalidate();
		}
	}

protected:
	int OnCreate(LPCREATESTRUCT)
	{
		SetWindowText( _T("SPC700 status") );

		m_hDC=GetDC();

		for(INT i=0;i<tabsize(m_hDCGroup);i++)
			m_hBmpGroup[i]=(HBITMAP)(m_hDCGroup[i]=NULL);
		for(INT i=0;i<tabsize(m_Colors);i++)
			m_Pens[i]=(HPEN)(m_Brushes[i]=NULL);
		m_BackBrush=(HBRUSH)(m_hFont=NULL);

		LOGFONT lf;
		lf.lfHeight=-11;
		lf.lfWidth=0;
		lf.lfEscapement=0;
		lf.lfOrientation=0;
		lf.lfWeight=FW_REGULAR;
		lf.lfItalic=0;
		lf.lfUnderline=0;
		lf.lfStrikeOut=0;
		lf.lfCharSet=ANSI_CHARSET;
		lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
		lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
		lf.lfQuality=DEFAULT_QUALITY;
		lf.lfPitchAndFamily=VARIABLE_PITCH|FF_SWISS;
		_tcscpy(lf.lfFaceName,_T("MS Sans Serif"));

		m_hFont=CreateFontIndirect(&lf);
		if(!m_hFont)
			return FALSE;

		m_hOldFont=(HFONT)SelectObject(m_hDC,m_hFont);
		SetBkMode(m_hDC,TRANSPARENT);

		if(!Refresh())
			return FALSE;

		static_api_ptr_t<play_callback_manager>()->register_callback( this, play_callback::flag_on_playback_new_track | play_callback::flag_on_playback_stop | play_callback::flag_on_playback_dynamic_info, false );

		CenterWindow();

		//cfg_placement.on_window_creation( m_hWnd );

		return TRUE;
	}

	void OnDestroy()
	{
		m_bVisRunning = false;
		m_bDrawAll = TRUE;
		Render();

		//cfg_placement.on_window_destruction( m_hWnd );

		static_api_ptr_t<play_callback_manager>()->unregister_callback( this );

		SelectObject(m_hDC,m_hOldFont);
		ReleaseDC(m_hDC);
		for(INT i=0;i<tabsize(m_Colors);i++)
		{
			if(m_Pens[i])DeleteObject(m_Pens[i]);
			if(m_Brushes[i])DeleteObject(m_Brushes[i]);
		}
		if(m_BackBrush)DeleteObject(m_BackBrush);
		for(INT i=0;i<tabsize(m_hDCGroup);i++)
		{
			if(m_hDCGroup[i])DeleteDC(m_hDCGroup[i]);
			if(m_hBmpGroup[i])DeleteObject(m_hBmpGroup[i]);
		}
		if(m_hFont)DeleteObject(m_hFont);
	}

	void OnPaint(CDCHandle dc)
	{
		RECT rc;
		GetClientRect(&rc);
		FillRect(m_hDC,&rc,m_BackBrush);
		if(m_bVisShowLabels)
		{
			SetTextColor(m_hDC,m_Colors[1]);
			for(INT i=0;i<tabsize(m_Columns);i++)
				TextOut(m_hDC,m_Columns[i].x,m_Columns[i].y,m_Columns[i].szName,_tcslen(m_Columns[i].szName));
		}
		m_bDrawAll = TRUE;
		Render();
		ValidateRect(NULL);
	}

private:
	BOOL Line(HDC hdc,COLORREF cr,int nXStart,int nYStart,int nXEnd,int nYEnd)
	{
		HPEN hPen=CreatePen(PS_SOLID,1,cr);
		if(!hPen)return FALSE;
		HPEN hOldPen=(HPEN)SelectObject(hdc,hPen);
		if(!hOldPen)return FALSE;
		if(!MoveToEx(hdc,nXStart,nYStart,NULL))return FALSE;
		if(!LineTo(hdc,nXEnd,nYEnd))return FALSE;
		SelectObject(hdc,hOldPen);
		DeleteObject(hPen);
		return TRUE;
	}

	void DrawChannelGauge(HDC pDC, INT nXPos, INT nYPos, INT nWidth, INT nHeight,COLORREF clr0,COLORREF clr1)
	{
		RECT rt;
		rt.left=nXPos;
		rt.top=nYPos;
		rt.right=nXPos+nWidth;
		rt.bottom=nYPos+nHeight;

		for(INT x=rt.left;x<=rt.right;x++)
		{
			double pos=(double)(x-rt.left)/nWidth;
			INT r=(GetRValue(clr1)-GetRValue(clr0))*pos+GetRValue(clr0);
			if(r>255)r=255;
			INT g=(GetGValue(clr1)-GetGValue(clr0))*pos+GetGValue(clr0);
			if(g>255)g=255;
			INT b=(GetBValue(clr1)-GetBValue(clr0))*pos+GetBValue(clr0);
			if(b>255)b=255;
			Line(pDC,RGB(r,g,b),x,rt.top,x,rt.bottom);
		}
	}

protected:
	BOOL Refresh()
	{
		memcpy(&m_Colors,&ColorPreset[m_bVisColorStyle],sizeof(m_Colors));

		SIZE sz;
		for(UINT i=0;i<tabsize(m_Columns);i++)
		{
			if(!i)
				m_Columns[i].x=0;
			else
			{			
				GetTextExtentPoint32(m_hDC,m_Columns[i-1].szName,_tcslen(m_Columns[i-1].szName),&sz);
				m_Columns[i].x=m_Columns[i-1].x+sz.cx+5;
			}
			m_Columns[i].y=0;
		}
		if(!m_bVisShowLabels)
		{
			GetTextExtentPoint32(m_hDC,_T("0"),_tcslen(_T("0")),&sz);
			m_Columns[1].x=m_Columns[0].x+sz.cx+5;
			GetTextExtentPoint32(m_hDC,_T("000"),_tcslen(_T("000")),&sz);
			m_Columns[2].x=m_Columns[1].x+sz.cx+5;
			m_Columns[3].x=m_Columns[2].x+18;
			m_Columns[4].x=m_Columns[3].x+18;
			m_Columns[5].x=m_Columns[4].x+18;
			m_Columns[6].x=m_Columns[5].x+18;
			m_Columns[7].x=m_Columns[6].x+18;
			GetTextExtentPoint32(m_hDC,_T("Release"),_tcslen(_T("Release")),&sz);
			m_Columns[8].x=m_Columns[7].x+sz.cx+5;
		}
		m_Columns[9].x=m_Columns[8].x+55;
		m_Columns[10].x=m_Columns[9].x+55;
		m_Columns[11].x=m_Columns[10].x+55;

		for(UINT i=0;i<tabsize(m_Colors);i++)
		{
			if(m_Pens[i])DeleteObject(m_Pens[i]);
			m_Pens[i]=CreatePen(PS_SOLID,1,m_Colors[i]);
			if(m_Brushes[i])DeleteObject(m_Brushes[i]);
			m_Brushes[i]=CreateSolidBrush(m_Colors[i]);
		}

		for(UINT i=0;i<3;i++)
		{
			if(m_hDCGroup[i*2])DeleteDC(m_hDCGroup[i*2]);
			m_hDCGroup[i*2]=CreateCompatibleDC(m_hDC);
			if(m_hBmpGroup[i*2])DeleteObject(m_hBmpGroup[i*2]);
			m_hBmpGroup[i*2]=CreateCompatibleBitmap(m_hDC,50,13);
			SelectObject(m_hDCGroup[i*2],m_hBmpGroup[i*2]);
			INT r0=GetRValue(m_Colors[(i+8)*2])*0.7;
			INT g0=GetGValue(m_Colors[(i+8)*2])*0.7;
			INT b0=GetBValue(m_Colors[(i+8)*2])*0.7;
			INT r1=GetRValue(m_Colors[(i+8)*2+1])*0.7;
			INT g1=GetGValue(m_Colors[(i+8)*2+1])*0.7;
			INT b1=GetBValue(m_Colors[(i+8)*2+1])*0.7;
			DrawChannelGauge(m_hDCGroup[i*2],0,0,50,13,RGB(r0,g0,b0),RGB(r1,g1,b1));

			if(m_hDCGroup[i*2+1])DeleteDC(m_hDCGroup[i*2+1]);
			m_hDCGroup[i*2+1]=CreateCompatibleDC(m_hDC);
			if(m_hBmpGroup[i*2+1])DeleteObject(m_hBmpGroup[i*2+1]);
			m_hBmpGroup[i*2+1]=CreateCompatibleBitmap(m_hDC,50,13);
			SelectObject(m_hDCGroup[i*2+1],m_hBmpGroup[i*2+1]);
			r0=GetRValue(m_Colors[(i+8)*2])*1.3;
			if(r0>255)r0=255;
			g0=GetGValue(m_Colors[(i+8)*2])*1.3;
			if(g0>255)g0=255;
			b0=GetBValue(m_Colors[(i+8)*2])*1.3;
			if(b0>255)b0=255;
			r1=GetRValue(m_Colors[(i+8)*2+1])*1.3;
			if(r1>255)r1=255;
			g1=GetGValue(m_Colors[(i+8)*2+1])*1.3;
			if(g1>255)g1=255;
			b1=GetBValue(m_Colors[(i+8)*2+1])*1.3;
			if(b1>255)b1=255;
			DrawChannelGauge(m_hDCGroup[i*2+1],0,0,50,13,RGB(r0,g0,b0),RGB(r1,g1,b1));
		}
		if(m_hDCGroup[6])DeleteDC(m_hDCGroup[6]);
		m_hDCGroup[6]=CreateCompatibleDC(m_hDC);
		if(m_hBmpGroup[6])DeleteObject(m_hBmpGroup[6]);
		m_hBmpGroup[6]=CreateCompatibleBitmap(m_hDC,50,13);
		SelectObject(m_hDCGroup[6],m_hBmpGroup[6]);

		HDC hDC=CreateCompatibleDC(m_hDC);
		HBITMAP hBmp=CreateCompatibleBitmap(m_hDC,8,8);
		SelectObject(hDC,hBmp);

		SetClassLong(m_hWnd,GCL_HBRBACKGROUND,(LONG)NULL);
		if(m_BackBrush)DeleteObject(m_BackBrush);
		m_BackBrush=CreateSolidBrush(m_Colors[0]);
		SetClassLong(m_hWnd,GCL_HBRBACKGROUND,(LONG)m_BackBrush);

		m_bDrawAll = TRUE;
		Invalidate();

		return TRUE;
	}

private:
	BOOL Render()
	{
		RECT rc;
		INT nYPos=m_bVisShowLabels?15:1;
		for(INT i=0;i<8;i++)
		{
			if(m_bDrawAll)
			{
				SetRect(&rc,m_Columns[0].x,i*14+nYPos,m_Columns[1].x-1,i*14+nYPos+13);
				FillRect(m_hDC,&rc,m_BackBrush);
				SetTextColor(m_hDC,m_Colors[m_bVisRunning?3:2]);
				TCHAR szBuf[2];
				_itot(i,szBuf,10);
				TextOut(m_hDC,m_Columns[0].x,i*14+nYPos,szBuf,_tcslen(szBuf));
			}

			BYTE bWave,bOldWave;
			bWave = regs[ i * 0x10 + Spc_Dsp::v_srcn ]; bOldWave = old_regs[ i * 0x10 + Spc_Dsp::v_srcn ];
			if(m_bDrawAll||bWave!=bOldWave)
			{
				SetRect(&rc,m_Columns[1].x,i*14+nYPos,m_Columns[2].x-1,i*14+nYPos+13);
				FillRect(m_hDC,&rc,m_BackBrush);
				TCHAR szBuf[20];
				_stprintf(szBuf,_T("%u"),m_bVisRunning?bWave:0);
				SetTextColor(m_hDC,m_Colors[m_bVisRunning?5:4]);
				TextOut(m_hDC,m_Columns[1].x,i*14+nYPos,szBuf,_tcslen(szBuf));
			}

			if(m_bDrawAll||((regs[Spc_Dsp::r_kon]^old_regs[Spc_Dsp::r_kon])&(1<<i)))
			{
				SetRect(&rc,m_Columns[2].x,i*14+nYPos,m_Columns[2].x+13,i*14+nYPos+13);
				INT br=m_bVisRunning&&regs[Spc_Dsp::r_kon]&(1<<i)?7:6;
				FillRect(m_hDC,&rc,m_Brushes[br]);
			}

			if(m_bDrawAll||((regs[Spc_Dsp::r_eon]^old_regs[Spc_Dsp::r_eon])&(1<<i)))
			{
				SetRect(&rc,m_Columns[3].x,i*14+nYPos,m_Columns[3].x+13,i*14+nYPos+13);
				INT br=m_bVisRunning&&regs[Spc_Dsp::r_eon]&(1<<i)?9:8;
				FillRect(m_hDC,&rc,m_Brushes[br]);
			}

			if(m_bDrawAll||((regs[Spc_Dsp::r_pmon]^old_regs[Spc_Dsp::r_pmon])&(1<<i)))
			{
				SetRect(&rc,m_Columns[4].x,i*14+nYPos,m_Columns[4].x+13,i*14+nYPos+13);
				INT br=m_bVisRunning&&regs[Spc_Dsp::r_pmon]&(1<<i)?11:10;
				FillRect(m_hDC,&rc,m_Brushes[br]);
			}

			if(m_bDrawAll||((regs[Spc_Dsp::r_non]^old_regs[Spc_Dsp::r_non])&(1<<i)))
			{
				SetRect(&rc,m_Columns[5].x,i*14+nYPos,m_Columns[5].x+13,i*14+nYPos+13);
				INT br=m_bVisRunning&&regs[Spc_Dsp::r_non]&(1<<i)?13:12;
				FillRect(m_hDC,&rc,m_Brushes[br]);
			}

			BYTE bESrnd=(regs[Spc_Dsp::r_evoll]^regs[Spc_Dsp::r_evolr])&0x80,bOldESrnd=(old_regs[Spc_Dsp::r_evoll]^old_regs[Spc_Dsp::r_evolr])&0x80;
			BYTE bSrnd=(regs[i*0x10+Spc_Dsp::v_voll]^regs[i*0x10+Spc_Dsp::v_volr]),bOldSrnd=(old_regs[i*0x10+Spc_Dsp::v_voll]^old_regs[i*0x10+Spc_Dsp::v_volr]);
			bSrnd=(bSrnd^(regs[Spc_Dsp::r_mvoll]^regs[Spc_Dsp::r_mvolr]))&0x80;
			bOldSrnd=(bOldSrnd^(old_regs[Spc_Dsp::r_mvoll]^old_regs[Spc_Dsp::r_mvolr]))&0x80;
			bSrnd=max(bSrnd,bESrnd);
			bOldSrnd=max(bOldSrnd,bOldESrnd);
			if(m_bDrawAll||bSrnd!=bOldSrnd)
			{
				SetRect(&rc,m_Columns[6].x,i*14+nYPos,m_Columns[6].x+13,i*14+nYPos+13);
				INT br=m_bVisRunning&&bSrnd?15:14;
				FillRect(m_hDC,&rc,m_Brushes[br]);
			}

			BOOL playingchanged=env_modes[i]!=old_env_modes[i]||env_modes[i]!=Sustain||(!regs[i*0x10+Spc_Dsp::v_envx])!=(!old_regs[i*0x10+Spc_Dsp::v_envx]);
			BOOL active=regs[i*0x10+Spc_Dsp::v_envx]||env_modes[i]!=Release;

			if(m_bDrawAll||playingchanged)
			{
				SetRect(&rc,m_Columns[7].x,i*14+nYPos,m_Columns[8].x-1,i*14+nYPos+13);
				FillRect(m_hDC,&rc,m_BackBrush);
				if(m_bVisRunning&&active)
				{
					for(INT e=0;e<sizeof(m_EnvTypes)/sizeof(m_EnvTypes[0]);e++)
					{
						if(env_modes[i]==m_EnvTypes[e].mode)
						{
							SetTextColor(m_hDC,m_EnvTypes[e].cr);
							TextOut(m_hDC,m_Columns[7].x,i*14+nYPos,m_EnvTypes[e].szName,_tcslen(m_EnvTypes[e].szName));
							break;
						}
					}
				}
				else
				{
					SetTextColor(m_hDC,m_Colors[4]);
					TextOut(m_hDC,m_Columns[7].x,i*14+nYPos,_T("Off"),_tcslen(_T("Off")));
				}
			}

			if(m_bDrawAll||regs[i*0x10+Spc_Dsp::v_envx]!=old_regs[i*0x10+Spc_Dsp::v_envx])
			{
				INT w=regs[i*0x10+Spc_Dsp::v_envx]*50/127;
				BitBlt(m_hDCGroup[6],0,0,50,13,m_hDCGroup[0],0,0,SRCCOPY);
				if(m_bVisRunning&&w)
					BitBlt(m_hDCGroup[6],0,0,w,13,m_hDCGroup[1],0,0,SRCCOPY);
				BitBlt(m_hDC,m_Columns[8].x,i*14+nYPos,50,13,m_hDCGroup[6],0,0,SRCCOPY);
			}

			if(m_bDrawAll||playingchanged||regs[i*0x10+Spc_Dsp::v_pitchl]!=old_regs[i*0x10+Spc_Dsp::v_pitchl]||((regs[i*0x10+Spc_Dsp::v_pitchh]^old_regs[i*0x10+Spc_Dsp::v_pitchh])&0x3f))
			{
				INT pitch=regs[i*0x10+Spc_Dsp::v_pitchl]|(regs[i*0x10+Spc_Dsp::v_pitchh]<<8)&0x3FFF;
				INT w=pitch*50/0x3FFF;
				BitBlt(m_hDCGroup[6],0,0,50,13,m_hDCGroup[2],0,0,SRCCOPY);
				if(m_bVisRunning&&w&&active)
					BitBlt(m_hDCGroup[6],0,0,w,13,m_hDCGroup[3],0,0,SRCCOPY);
				BitBlt(m_hDC,m_Columns[9].x,i*14+nYPos,50,13,m_hDCGroup[6],0,0,SRCCOPY);
			}

			if(m_bDrawAll||playingchanged||out_levels[i*2+0]!=old_out_levels[i*2+0]||out_levels[i*2+1]!=old_out_levels[i*2+1])
			{
				INT out=max(out_levels[i*2+0],out_levels[i*2+1]);
				float db=log((float)out)/log(2.0f);
				INT w=(INT)(((db*6.0f+6.0f)-48.0f)*50.0f/48.0f);
				if(w<0)w=0;
				BitBlt(m_hDCGroup[6],0,0,50,13,m_hDCGroup[4],0,0,SRCCOPY);
				if(m_bVisRunning&&w&&active)
					BitBlt(m_hDCGroup[6],0,0,w,13,m_hDCGroup[5],0,0,SRCCOPY);
				BitBlt(m_hDC,m_Columns[10].x,i*14+nYPos,50,13,m_hDCGroup[6],0,0,SRCCOPY);
			}
		}

		memcpy(&old_regs,&regs,sizeof(old_regs));
		memcpy(&old_env_modes,&env_modes,sizeof(old_env_modes));

		m_bDrawAll = FALSE;

		return 0;
	}
};

/*class CVisWindowPopup : public CVisWindow
{
public:
	BEGIN_MSG_MAP(CVisWindow)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_PAINT(CVisWindow::OnPaint)
		MSG_WM_CONTEXTMENU(OnContextMenu)
		MSG_WM_DESTROY(OnDestroy)
	END_MSG_MAP()

	CVisWindowPopup() : CVisWindow() {}

	void UpdateLayout(BOOL bResizeBars = TRUE)
	{
		Refresh();

		RECT rcw = { 0, 0, m_Columns[11].x, Spc_Dsp::voice_count * 14 + 2 + ( m_bVisShowLabels ? 14 : 0 ) };

		::AdjustWindowRectEx( &rcw, GetStyle(), FALSE, GetExStyle() );

		SetWindowPos( NULL, &rcw, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE );
	}

	int OnCreate(LPCREATESTRUCT lpcs)
	{
		m_bVisShowLabels = cfg_labels;
		m_bVisColorStyle = cfg_style;

		return CVisWindow::OnCreate(lpcs);
	}

	void OnDestroy()
	{
		CVisWindow::OnDestroy();

		cfg_labels = m_bVisShowLabels;
		cfg_style = m_bVisColorStyle;
	}

	void OnContextMenu(CWindow wnd, CPoint point)
	{
		HMENU hMenu=CreatePopupMenu();
		if(!hMenu) return;
		AppendMenu(hMenu,MF_STRING|(m_bVisShowLabels?MF_CHECKED:0),1,_T("Column &Labels"));
		AppendMenu(hMenu,MF_SEPARATOR,0,0);
		AppendMenu(hMenu,MF_STRING,2,_T("Color style &1"));
		AppendMenu(hMenu,MF_STRING,3,_T("Color style &2"));
		AppendMenu(hMenu,MF_STRING,4,_T("Color style &3"));
		AppendMenu(hMenu,MF_STRING,5,_T("Color style &4"));
		if (point.x == 0xFFFF && point.y == 0xFFFF)
		{
			point.x = 0; point.y = 0;
			wnd.ClientToScreen(&point);
		}
		INT nSelection=TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_NONOTIFY|TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,wnd,NULL);
		DestroyMenu(hMenu);

		switch(nSelection)
		{
		case 1:
			m_bVisShowLabels = !m_bVisShowLabels;
			UpdateLayout();
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			m_bVisColorStyle = nSelection - 2;
			UpdateLayout();
			break;
		}
	}
};*/

// {B08E684B-94EB-4AE6-84E1-EB40AF3BE4F2}
static const GUID guid_ui_element = 
{ 0xb08e684b, 0x94eb, 0x4ae6, { 0x84, 0xe1, 0xeb, 0x40, 0xaf, 0x3b, 0xe4, 0xf2 } };

class CVisWindowElementConfig : public ui_element_config
{
public:
	struct data
	{
		bool show_labels;
		unsigned char color_style;
		data() : show_labels(true), color_style(0) {}
	} the_data;

	virtual GUID get_guid() const { return guid_ui_element; }
	virtual const void * get_data() const { return &the_data; }
	virtual t_size get_data_size() const { return sizeof(the_data); }
};

class CVisWindowElementInstance;

critical_section g_VisWindows_sync;
pfc::ptr_list_t<CVisWindowElementInstance> g_VisWindows;

class CVisWindowElementInstance : public CVisWindow, public ui_element_instance
{
	ui_element_instance_callback_ptr m_callback;

public:
	BEGIN_MSG_MAP(CVisWindow)
		MSG_WM_CONTEXTMENU(OnContextMenu)
		CHAIN_MSG_MAP(CVisWindow)
	END_MSG_MAP()

	CVisWindowElementInstance(HWND hwndParent, ui_element_config::ptr cfg, ui_element_instance_callback_ptr p_callback) : CVisWindow()
	{
		m_callback = p_callback;
		m_bVisShowLabels = true;
		m_bVisColorStyle = 0;
		if ( cfg->get_guid() == get_guid() && cfg->get_data_size() == sizeof(CVisWindowElementConfig::data) )
		{
			const CVisWindowElementConfig::data * data = (const CVisWindowElementConfig::data *) cfg->get_data();
			m_bVisShowLabels = data->show_labels;
			m_bVisColorStyle = data->color_style;
		}
		Create( hwndParent, 0, 0, 0, 0, 0U, 0 );

		{
			insync(g_VisWindows_sync);
			g_VisWindows.add_item(this);
		}
	}

	~CVisWindowElementInstance()
	{
		if (m_hWnd) DestroyWindow();
		{
			insync(g_VisWindows_sync);
			g_VisWindows.remove_item(this);
		}
	}

	void UpdateLayout()
	{
		Refresh();

		RECT rcw = { 0, 0, m_Columns[11].x, Spc_Dsp::voice_count * 14 + 2 + ( m_bVisShowLabels ? 14 : 0 ) };

		::AdjustWindowRectEx( &rcw, GetStyle(), FALSE, GetExStyle() );

		SetWindowPos( NULL, &rcw, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE );

		m_callback->on_min_max_info_change();
	}

	virtual HWND get_wnd() { return m_hWnd; }

	virtual void set_configuration(ui_element_config::ptr data)
	{
		if ( data->get_guid() == get_guid() && data->get_data_size() == sizeof(CVisWindowElementConfig::data) )
		{
			const CVisWindowElementConfig::data * _data = (const CVisWindowElementConfig::data *) data->get_data();
			m_bVisShowLabels = _data->show_labels;
			m_bVisColorStyle = _data->color_style;
			UpdateLayout();
		}
	}

	virtual ui_element_config::ptr get_configuration()
	{
		CVisWindowElementConfig * config = new service_impl_t<CVisWindowElementConfig>;
		config->the_data.show_labels = m_bVisShowLabels;
		config->the_data.color_style = m_bVisColorStyle;
		return config;
	}

	virtual GUID get_guid() { return guid_ui_element; }
	virtual GUID get_subclass() { return guid_ui_element; }

	virtual ui_element_min_max_info get_min_max_info()
	{
		ui_element_min_max_info ret;
		ret.m_min_width = m_Columns[11].x;
		ret.m_max_width = 1024 * 1024;
		ret.m_min_height = Spc_Dsp::voice_count * 14 + 2 + 14 * m_bVisShowLabels;
		ret.m_max_height = Spc_Dsp::voice_count * 14 + 2 + 14 * m_bVisShowLabels;
		return ret;
	}

	virtual bool edit_mode_context_menu_test(const POINT & p_point,bool p_fromkeyboard) {return true;}
	virtual void edit_mode_context_menu_build(const POINT & p_point,bool p_fromkeyboard,HMENU p_menu,unsigned p_id_base)
	{
		AppendMenu(p_menu,MF_STRING|(m_bVisShowLabels?MF_CHECKED:0),p_id_base+1,_T("Column &Labels"));
		AppendMenu(p_menu,MF_SEPARATOR,0,0);
		AppendMenu(p_menu,MF_STRING,p_id_base+2,_T("Color style &1"));
		AppendMenu(p_menu,MF_STRING,p_id_base+3,_T("Color style &2"));
		AppendMenu(p_menu,MF_STRING,p_id_base+4,_T("Color style &3"));
		AppendMenu(p_menu,MF_STRING,p_id_base+5,_T("Color style &4"));
	}
	virtual void edit_mode_context_menu_command(const POINT & p_point,bool p_fromkeyboard,unsigned p_id,unsigned p_id_base)
	{
		switch (p_id - p_id_base)
		{
		case 1:
			m_bVisShowLabels = !m_bVisShowLabels;
			UpdateLayout();
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			m_bVisColorStyle = p_id - p_id_base - 2;
			UpdateLayout();
			break;
		}
	}
	virtual bool edit_mode_context_menu_get_focus_point(POINT & p_point)
	{
		p_point.x = 0;
		p_point.y = 0;
		ClientToScreen(&p_point);
		return true;
	}
	virtual bool edit_mode_context_menu_get_description(unsigned p_id,unsigned p_id_base,pfc::string_base & p_out)
	{
		switch (p_id - p_id_base)
		{
		case 1:
			p_out = "Toggles the display of the column labels.";
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			p_out = "Changes the current color style.";
			break;
		default:
			return false;
		}
		return true;
	}

	void OnContextMenu(CWindow wnd, CPoint point)
	{
		if ( !m_callback->is_edit_mode_enabled() )
		{
			HMENU hMenu=CreatePopupMenu();
			if(!hMenu) return;
			edit_mode_context_menu_build( point, false, hMenu, 0 );
			if (point.x == -1 && point.y == -1)
			{
				point.x = 0; point.y = 0;
				wnd.ClientToScreen(&point);
			}
			INT nSelection=TrackPopupMenu(hMenu,TPM_NONOTIFY|TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,wnd,NULL);
			DestroyMenu(hMenu);
			edit_mode_context_menu_command( point, false, nSelection, 0 );
		}
		else
		{
			SetMsgHandled(FALSE);
		}
	}

	void Activate()
	{
		m_callback->request_activation( this );
	}
};

class CVisWindowElement : public ui_element_v2
{
	virtual GUID get_guid() { return guid_ui_element; }
	virtual GUID get_subclass() { return ui_element_subclass_playback_visualisation; }
	virtual void get_name(pfc::string_base & p_out) { p_out = "SPC700 status"; }
	virtual ui_element_instance_ptr instantiate(HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback)
	{
		return new service_impl_t<CVisWindowElementInstance>( p_parent, cfg, p_callback );
	}
	virtual ui_element_config::ptr get_default_configuration() { return new service_impl_t<CVisWindowElementConfig>; }
	virtual ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) { return NULL; }
	virtual bool get_description(pfc::string_base & p_out) { p_out = "Displays the status of the emulated SPC700 while a SPC file is playing."; return true; }
	virtual t_uint32 get_flags() { return KFlagSupportsBump | KFlagHavePopupCommand; }
	virtual bool bump()
	{
		insync(g_VisWindows_sync);
		if (g_VisWindows.get_count())
		{
			for ( unsigned i = 0, j = g_VisWindows.get_count(); i < j; i++ )
				g_VisWindows[ i ]->Activate();
			return true;
		}
		else return false;
	}
};

/*CVisWindowPopup g_VisWindow;

class spc_vis_menu : public mainmenu_commands
{
	virtual t_uint32 get_command_count()
	{
		return 1;
	}

	virtual GUID get_command(t_uint32 p_index)
	{
		// {98A244AC-D9CC-4AA5-8471-48FD0D1BCAC1}
		static const GUID guid = 
		{ 0x98a244ac, 0xd9cc, 0x4aa5, { 0x84, 0x71, 0x48, 0xfd, 0xd, 0x1b, 0xca, 0xc1 } };
		return guid;
	}
	
	virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
	{
		p_out = "SPC700 status";
	}

	virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
	{
		p_out = "Activates the Game Emu Player SPC700 status window.";
		return true;
	}

	virtual GUID get_parent()
	{
		return mainmenu_groups::view_visualisations;
	}

	virtual bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags)
	{
		p_flags = 0;
		get_name(p_index,p_text);
		return true;
	}

	virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
	{
		if ( p_index == 0 && core_api::assert_main_thread() )
		{
			if ( !g_VisWindow.IsWindow() )
			{
				try
				{
					g_VisWindow.CreateEx( core_api::get_main_window(), 0, WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU, WS_EX_TOOLWINDOW, 0 );
				}
				catch ( const std::exception & e )
				{
					console::error( e.what() );
				}
			}
		}
	}
};

static mainmenu_commands_factory_t <spc_vis_menu> g_mainmenu_commands_spc_vis_factory;*/

static service_factory_single_t<CVisWindowElement> g_element_spc_vis_factory;
