
class
SEdit : public SControl
{
public:
	SEdit(SWindow* parent, int id);
	SEdit(SWindow* parent, HWND hwnd);
	~SEdit(void);

	void	SetSel(int nStart, int nEnd = -1);
	bool	ScrollCaret(void);
};

