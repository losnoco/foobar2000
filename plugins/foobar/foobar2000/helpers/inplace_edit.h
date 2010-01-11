namespace InPlaceEdit {

	enum {
		KEditAborted = 0,
		KEditTab,
		KEditShiftTab,
		KEditEnter,
		KEditLostFocus,

		KEditMaskReason = 0xFF,
		KEditFlagContentChanged = 0x100,
	};

	void Start(HWND p_parentwnd,const RECT & p_rect,bool p_multiline,pfc::rcptr_t<pfc::string_base> p_content,completion_notify_ptr p_notify);

	void Start_FromListView(HWND p_listview,unsigned p_item,unsigned p_subitem,unsigned p_linecount,pfc::rcptr_t<pfc::string_base> p_content,completion_notify_ptr p_notify);
	
	bool TableEditAdvance(unsigned & p_item,unsigned & p_column, unsigned p_item_count,unsigned p_column_count, unsigned p_whathappened);


	class CTableEditHelper {
	public:
		void TableEdit_Start(HWND p_listview,unsigned p_item,unsigned p_column,unsigned p_itemcount,unsigned p_columncount,unsigned p_basecolumn) {
			if (m_notify.is_valid()) return;
			m_listview = p_listview;
			m_item = p_item;
			m_column = p_column;
			m_itemcount = p_itemcount;
			m_columncount = p_columncount;
			m_basecolumn = p_basecolumn;
			__Start();
		}

		void TableEdit_Abort(bool p_forwardcontent) {
			if (m_notify.is_valid()) {
				m_notify->orphan();
				m_notify.release();

				if (p_forwardcontent) {
					if (m_content.is_valid()) {
						pfc::string8 temp(*m_content);
						m_content.release();
						TableEdit_SetItemText(m_item,m_column,temp);
					}
				} else {
					m_content.release();
				}
				SetFocus(NULL);
			}
		}


		bool TableEdit_IsActive() const {
			return m_notify.is_valid();
		}

		virtual bool TableEdit_GetItemText(unsigned p_item,unsigned p_column,pfc::string_base & p_out,unsigned & p_linecount) {
			listview_helper::get_item_text(m_listview,p_item,p_column + m_basecolumn,p_out);
			p_linecount = pfc::is_multiline(p_out) ? 5 : 1;
			return true;
		}
		virtual void TableEdit_SetItemText(unsigned p_item,unsigned p_column,const char * p_text) {
			listview_helper::set_item_text(m_listview,p_item,p_column + m_basecolumn,p_text);
		}
		
		void on_task_completion(unsigned p_taskid,unsigned p_state) {
			if (p_taskid == KTaskID) {
				m_notify.release();			
				if (m_content.is_valid()) {
					if (p_state & InPlaceEdit::KEditFlagContentChanged) {
						TableEdit_SetItemText(m_item,m_column,*m_content);
					}
					m_content.release();
				}
				if (InPlaceEdit::TableEditAdvance(m_item,m_column,m_itemcount,m_columncount,p_state)) {
					__Start();
				}
			}
		}

		~CTableEditHelper() {
			if (m_notify.is_valid()) {
				m_notify->orphan();
				m_notify.release();
			}
		}
	private:
		void __Start() {
			listview_helper::select_single_item(m_listview,m_item);
			m_content.new_t();
			unsigned linecount = 1;
			if (!TableEdit_GetItemText(m_item,m_column,*m_content,linecount)) return;
			m_notify = completion_notify_create(this,KTaskID);
			InPlaceEdit::Start_FromListView(m_listview,m_item,m_column+m_basecolumn,linecount,m_content,m_notify);
		}
		enum {
			KTaskID = 0xc0ffee
		};
		HWND m_listview;
		unsigned m_item,m_column;
		unsigned m_itemcount,m_columncount,m_basecolumn;
		pfc::rcptr_t<pfc::string8> m_content;
		service_ptr_t<completion_notify_orphanable> m_notify;
	};

}