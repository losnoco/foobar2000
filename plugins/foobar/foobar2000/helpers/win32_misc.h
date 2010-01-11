class registerclass_scope_delayed {
public:
	registerclass_scope_delayed() : m_class(0) {}

	bool is_registered() const {return m_class != 0;}
	void toggle_on(UINT p_style,WNDPROC p_wndproc,int p_clsextra,int p_wndextra,HICON p_icon,HCURSOR p_cursor,HBRUSH p_background,const TCHAR * p_classname,const TCHAR * p_menuname);
	void toggle_off();
	ATOM get_class() const {return m_class;}

	~registerclass_scope_delayed() {toggle_off();}
private:
	registerclass_scope_delayed(const registerclass_scope_delayed &) {throw pfc::exception_not_implemented();}
	const registerclass_scope_delayed & operator=(const registerclass_scope_delayed &) {throw pfc::exception_not_implemented();}

	ATOM m_class;
};


class win32_menu {
public:
	win32_menu(HMENU p_initval) : m_menu(p_initval) {}
	win32_menu() : m_menu(NULL) {}
	~win32_menu() {release();}
	void release() {
		if (m_menu != NULL) {
			DestroyMenu(m_menu);
			m_menu = NULL;
		}
	}
	void set(HMENU p_menu) {release(); m_menu = p_menu;}
	void create_popup() {
		release();
		SetLastError(NO_ERROR);
		m_menu = CreatePopupMenu();
		if (m_menu == NULL) throw exception_win32(GetLastError());
	}
	HMENU get() const {return m_menu;}
	HMENU detach() {return pfc::replace_t(m_menu,(HMENU)NULL);}
	
	bool is_valid() const {return m_menu != NULL;}
private:
	win32_menu(const win32_menu &) {throw pfc::exception_not_implemented();}
	const win32_menu & operator=(const win32_menu &) {throw pfc::exception_not_implemented();}

	HMENU m_menu;
};

class win32_font {
public:
	win32_font(HFONT p_initval) : m_font(p_initval) {}
	win32_font() : m_font(NULL) {}
	~win32_font() {release();}

	void release() {
		if (m_font != NULL) {
			DeleteObject(m_font); m_font = NULL;
		}
	}

	void set(HFONT p_font) {release(); m_font = p_font;}
	HFONT get() const {return m_font;}
	HFONT detach() {return pfc::replace_t(m_font,(HFONT)NULL);}

	void create(const t_font_description & p_desc) {
		SetLastError(NO_ERROR);
		HFONT temp = p_desc.create();
		if (temp == NULL) throw exception_win32(GetLastError());
		set(temp);
	}

	bool is_valid() const {return m_font != NULL;}

private:
	win32_font(const win32_font&) {throw pfc::exception_not_implemented();}
	const win32_font & operator=(const win32_font &) {throw pfc::exception_not_implemented();}

	HFONT m_font;
};