
class
SComboBoxEx : public SControl
{
public:
	SComboBoxEx(SWindow* parent, int id);
	~SComboBoxEx(void);

	bool	InsertItem(UINT index, PCOMBOBOXEXITEM ItemInfo);
	HWND	GetComboControl(void);
};

