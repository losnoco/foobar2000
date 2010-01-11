#ifndef _FOOBAR2000_HELPERS_WIN32_DIALOG_H_
#define _FOOBAR2000_HELPERS_WIN32_DIALOG_H_

namespace dialog_helper
{
	class dialog
	{
	protected:
		
		dialog() : wnd(0), m_is_modal(false) {}
		~dialog() { }

		virtual BOOL on_message(UINT msg,WPARAM wp,LPARAM lp)=0;

		void end_dialog(int code);

	public:
		inline HWND get_wnd() {return wnd;}

		int run_modal(unsigned id,HWND parent);

		HWND run_modeless(unsigned id,HWND parent);
	private:
		HWND wnd;
		static BOOL CALLBACK DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);

		bool m_is_modal;

		modal_dialog_scope m_modal_scope;
	};

	struct set_item_text_multi_param
	{
		unsigned id;
		const char * text;
	};

	void set_item_text_multi(HWND wnd,const set_item_text_multi_param * param,unsigned count);



	//! This class is meant to be instantiated on-stack, as a local variable. Using new/delete operators instead or even making this a member of another object works, but does not make much sense because of the way this works (single run() call).
	class dialog_modal
	{
	public:
		int run(unsigned p_id,HWND p_parent,HINSTANCE p_instance = core_api::get_my_instance());
	protected:
		virtual BOOL on_message(UINT msg,WPARAM wp,LPARAM lp)=0;

		inline dialog_modal() : m_wnd(0) {}
		void end_dialog(int p_code);
		inline HWND get_wnd() const {return m_wnd;}
	private:
		static BOOL CALLBACK DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);

		HWND m_wnd;
		modal_dialog_scope m_modal_scope;
	};


	//! This class is meant to be used with new/delete operators only. Destroying the window will result in object calling delete this. If object is deleted directly using delete operator, WM_DESTROY handler may not be called so it should not be used (use destructor of derived class instead).
	//! Classes derived from dialog_modeless must not be instantiated in any other way than operator new().
	class dialog_modeless
	{
	public:
		bool create(unsigned p_id,HWND p_parent,HINSTANCE p_instance = core_api::get_my_instance());
	protected:
		virtual BOOL on_message(UINT msg,WPARAM wp,LPARAM lp)=0;

		inline dialog_modeless() : m_wnd(0), m_destructor_status(destructor_none), m_is_in_create(false) {}
		inline HWND get_wnd() const {return m_wnd;}
		virtual ~dialog_modeless();
	private:
		static BOOL CALLBACK DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
		void on_window_destruction();
		
		BOOL on_message_wrap(UINT msg,WPARAM wp,LPARAM lp);

		HWND m_wnd;
		enum {destructor_none,destructor_normal,destructor_fromwindow} m_destructor_status;
		bool m_is_in_create;
	};

};

#endif