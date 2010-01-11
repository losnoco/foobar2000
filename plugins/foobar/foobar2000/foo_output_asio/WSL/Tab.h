
class
STab : public SControl
{
public:
	STab(SWindow* parent, int id);
	~STab(void);

	bool	InsertItem(int iItem, TC_ITEM* pitem);
	bool	SetItem(int iItem, TC_ITEM* pitem);
	bool	Add(char* text);
	int		GetItemCount(void);
	int		SetCurSel(int iItem);
	int		GetCurSel(void);
};

