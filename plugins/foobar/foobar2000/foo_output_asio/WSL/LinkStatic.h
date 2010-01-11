
class
SLinkStatic : public SStatic
{
public:
	SLinkStatic(SWindow* parent, int id, int _IdCursor);
	~SLinkStatic(void);

private:
	HFONT	hFont;
	int		IdCursor;

	void	SetFontHdc(HDC hdc, LPRECT Rect, char* Text, int* TextLen);

protected:
	void	Paint(HDC hdc);
	void	WmPaint(Org_Mes* OrgMes);
	void	WmSetCursor(Org_Mes* OrgMes, HWND hwnd, UINT nHittest, UINT wMouseMsg);
};

