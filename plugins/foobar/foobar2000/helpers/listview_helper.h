namespace listview_helper
{
	unsigned insert_item(HWND p_listview,unsigned p_index,const char * p_name,LPARAM p_param);//returns index of new item on success, infinite on failure

	unsigned insert_column(HWND p_listview,unsigned p_index,const char * p_name,unsigned p_width_dlu);//returns index of new item on success, infinite on failure

	bool set_item_text(HWND p_listview,unsigned p_index,unsigned p_column,const char * p_name);

	bool is_item_selected(HWND p_listview,unsigned p_index);

	bool set_item_selection(HWND p_listview,unsigned p_index,bool p_state);

	bool select_single_item(HWND p_listview,unsigned p_index);

	bool ensure_visible(HWND p_listview,unsigned p_index);
};