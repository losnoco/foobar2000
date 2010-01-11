
enum
{
	BF_CHECKED   = 1,
	BF_GRAYED    = 2,
	BF_UNCHECKED = 0,
};

class
SButton : public SControl
{
public:
	SButton(SWindow* parent, int id);
	SButton(SWindow* parent, int id, DWORD AddStyle);
	~SButton(void);

	bool	SetCheck(int fCheck);
	bool	Check(void);
	bool	Uncheck(void);
	int		GetCheck(void);
};

