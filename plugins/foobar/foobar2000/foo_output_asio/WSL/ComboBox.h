
class
SComboBox : public SControl
{
public:
	SComboBox(SWindow* parent, int id);
	SComboBox(SWindow* parent, HWND hwnd);
	~SComboBox(void);

	int		GetCount(void);
	int		AddString(LPCSTR lpsz);
	int		SetEditSel(DWORD ichStart, DWORD ichEnd);
	DWORD	GetEditSel(LPDWORD lpdwStart, LPDWORD lpdwEnd);
	int		SetCurSel(int index);
	int		GetCurSel(void);
	int		DeleteString(int index);
};

