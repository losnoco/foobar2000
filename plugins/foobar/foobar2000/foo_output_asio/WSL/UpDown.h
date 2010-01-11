
class
SUpDown : public SControl
{
public:
	SUpDown(SWindow* parent, int id);
	~SUpDown(void);

	void	SetRange(short int nLower, short int nUpper);
	short int	SetPos(short nPos);
	short int	GetPos(void);
};

