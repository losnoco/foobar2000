#include <windows.h>

typedef struct window_hook_info
{
	DWORD dwThreadId;
	HHOOK hGetMsg;
	HWND hWnd;
} window_hook_info;

class window_hook
{
	bool added;
	window_hook_info whi;

	static LRESULT CALLBACK MouseProc(int code, WPARAM wp, LPARAM lp);
	static LRESULT CALLBACK GetMsgProc(int code, WPARAM wp, LPARAM lp);

public:
	window_hook();
	~window_hook();

	void Hook(HWND hWnd);
};
