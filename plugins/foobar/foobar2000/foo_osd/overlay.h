#ifndef _OSD_OVERLAY_H_
#define _OSD_OVERLAY_H_

#include "config.h"

class dissolve;

class COsdWnd
{
public:
	enum STATE {
		HIDDEN,
		FADING_IN,
		VISIBLE,
		FADING_OUT
	};

	COsdWnd(osd_state & _state);
	~COsdWnd();

	bool Initialize();
	void Post(const char * msg, bool interval);
	void PostVolume(int volume);
	void Repost(const char * msg);
	void Hide();

	STATE GetState();

	bool DoInterval();

private:
	bool Setup();
	void Setup2();
	void Update();
	void HideInternal();

public:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	void Repaint();
	bool RepaintVolume(int volume);

	HWND               m_hWnd;
	pfc::string_simple m_strText;
	POINT              m_pPos;
	SIZE               m_sSize;
	critical_section   m_bitmapsync;
	HBITMAP            m_hBitmap;
	BOOL               m_bFade;
	int                m_iFadeNow;
	int                m_iFadeTo;
	int                m_iFadeStep;

	dissolve         * m_dissolve;

	STATE			   m_sState;

	enum MODE {
		TEXT,
		VOLUME
	};

	MODE			  m_mMode;

	// volume
	int               m_iLastPostedVol;
	int				  m_iLastVol;
	int				  m_iLastVolQ;

	bool			  m_bInterval;

	osd_state       & m_state;
};

class string_utf8_nocolor : public pfc::string8
{
public:
	string_utf8_nocolor(const char * src, int len = -1);
};

#endif
