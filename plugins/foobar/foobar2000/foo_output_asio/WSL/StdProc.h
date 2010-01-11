

void	srandMT(unsigned long seed);
unsigned long	randMT(void);

// �傫������Ԃ�

template<class T> inline T
Max(T a, T b)
{
	return (a > b) ? a : b;
}

// ����������Ԃ�

template<class T> inline T
Min(T a, T b)
{
	return (a < b) ? a : b;
}

// SwapA��SwapB������

template<class T> inline void
Swap(T* SwapA, T* SwapB)
{
	T	Work = *SwapA;

	*SwapA = *SwapB;
	*SwapB = Work;
}

// string2����count���Astring1�ɃR�s�[

inline char*
Strncpy(char* string1, const char* string2, size_t count)
{
	size_t	CopyLen = Min(strlen(string2), count);

	memcpy(string1, string2, CopyLen);
	*(string1 + CopyLen) = '\0';

	return string1;
}

// �����̌n������Z�b�g

inline void
RandomSetSeed(unsigned long seed)
{
//	srand(seed);
	srandMT(seed);
}

// 0 �` (num - 1) �̗����𔭐�

inline int
Random(int num)
{
//	return static_cast<int>((static_cast<__int64>(rand()) * num) / (RAND_MAX + 1));
	return static_cast<int>((static_cast<unsigned __int64>(randMT()) * num) /
														(0xffffffffUi64 + 1Ui64));
}

