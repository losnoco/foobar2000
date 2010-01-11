
class
SReBar : public SControl
{
public:
	SReBar(SWindow* parent, int id);
	~SReBar(void);

	bool	InsertBand(UINT index, LPREBARBANDINFO BandInfo);
	bool	DeleteBand(UINT index);
	bool	SetBandInfo(UINT index, LPREBARBANDINFO BandInfo);
	bool	GetBandInfo(UINT index, LPREBARBANDINFO BandInfo);
	UINT	IdToIndex(UINT id);
	void	ShowBand(UINT index, bool show);
};

