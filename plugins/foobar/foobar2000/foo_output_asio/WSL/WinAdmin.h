
struct
SWindow_List
{
	SWindow_List*	Before;
	SWindow_List*	Next;

	bool	First;
	SWindow*	Window;
};

class
SWindowAdmin
{
public:
	SWindowAdmin(void);
	virtual	~SWindowAdmin(void);

	void	Add(SWindow* Window);
	void	Delete(SWindow* Window);
	bool	IsWindow(void);
	bool	IsFirstList(SWindow* Window);
	bool	IsFirst(SWindow* Window);

protected:
	SWindow_List*	List;

	SWindow_List*	GetLast(void);
	SWindow_List*	GetList(SWindow* Window);
};

