

void	srandMT(unsigned long seed);
unsigned long	randMT(void);

// 大きい方を返す

template<class T> inline T
Max(T a, T b)
{
	return (a > b) ? a : b;
}

// 小さい方を返す

template<class T> inline T
Min(T a, T b)
{
	return (a < b) ? a : b;
}

// SwapAとSwapBを交換

template<class T> inline void
Swap(T* SwapA, T* SwapB)
{
	T	Work = *SwapA;

	*SwapA = *SwapB;
	*SwapB = Work;
}

// string2からcount分、string1にコピー

inline char*
Strncpy(char* string1, const char* string2, size_t count)
{
	size_t	CopyLen = Min(strlen(string2), count);

	memcpy(string1, string2, CopyLen);
	*(string1 + CopyLen) = '\0';

	return string1;
}

// 乱数の系列をリセット

inline void
RandomSetSeed(unsigned long seed)
{
//	srand(seed);
	srandMT(seed);
}

// 0 ～ (num - 1) の乱数を発生

inline int
Random(int num)
{
//	return static_cast<int>((static_cast<__int64>(rand()) * num) / (RAND_MAX + 1));
	return static_cast<int>((static_cast<unsigned __int64>(randMT()) * num) /
														(0xffffffffUi64 + 1Ui64));
}

