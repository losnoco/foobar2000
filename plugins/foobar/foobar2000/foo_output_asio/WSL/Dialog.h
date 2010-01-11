
class
SDialog : public SWindow
{
public:
	SDialog(SWindow* parent, LPCSTR id);
	SDialog(SWindow* parent, int id);
	SDialog(HWND parent, LPCSTR id);
	SDialog(HWND parent, int id);
	~SDialog(void);

protected:
	virtual void	CmOk(void);
	virtual void	CmCancel(void);

	void	WmCommand(Org_Mes* OrgMes, int wNotifyCode, int wID, HWND hwndCtl);
};

