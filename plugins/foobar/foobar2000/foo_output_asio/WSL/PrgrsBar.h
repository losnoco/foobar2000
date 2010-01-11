
class
SProgressBar : public SControl
{
public:
	SProgressBar(SWindow* parent, int id);
	~SProgressBar(void);

	UINT	SetRange(UINT nMinRange, UINT nMaxRange);
	UINT	SetPos(UINT nNewPos);
};

