#ifndef _WINDOW_PLACEMENT_HELPER_H_
#define _WINDOW_PLACEMENT_HELPER_H_

class cfg_window_placement : public cfg_var
{
public:
	bool on_window_creation(HWND window);//returns true if window position has been changed, false if not
	void on_window_destruction(HWND window);
	bool read_from_window(HWND window);
	virtual bool get_raw_data(write_config_callback * out);
	virtual void set_raw_data(const void * data,int size);
	cfg_window_placement(const GUID & p_guid);
private:
	mem_block_list<HWND> m_windows;
	WINDOWPLACEMENT m_data;
};


//void read_window_placement(cfg_struct_t<WINDOWPLACEMENT> & dst,HWND wnd);
//bool apply_window_placement(const cfg_struct_t<WINDOWPLACEMENT> & src,HWND wnd);

#endif //_WINDOW_PLACEMENT_HELPER_H_