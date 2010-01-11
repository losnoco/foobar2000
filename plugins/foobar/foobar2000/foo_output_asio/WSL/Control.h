
class
SControl : public SWindow
{
public:
	SControl(SWindow* parent, char* classname, int id);
	SControl(SWindow* parent, HWND hwnd);
	~SControl(void);
};

