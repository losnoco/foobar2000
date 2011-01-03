
//
//  汎用関数(ヘッダ)
//
//  Written by Otachan
//  http://www.aikis.or.jp/~otachan/
//

bool	CutPathFileName(const char* FullName, char* FileNamePath,
						char* FileName = NULL);
void	CutFileNameExt(const char* FullName, char* FileName, char* Ext = NULL);
char*	KanjiStrncpy(char* OutBuff, char* InBuff, int Len);
char*	WideToMultiStrcpy(char* OutBuff, WCHAR* InBuff,
						UINT OutBuffSize, UINT InBuffSize,
						bool UseBOM, bool BigEndian);
char*	UTF8ToMultiStrcpy(char* OutBuff, unsigned char* InBuff,
						UINT OutBuffSize, UINT InBuffSize);
UINT	WideStrSize(WCHAR* InBuff, UINT MaxSize);
bool	EraseSpace(char* FileName, int StrLen);

/*
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
*/

// srcから3bytes分、destにコピー

inline void
Memcpy3bytes(unsigned char* dest, unsigned char* src)
{
	*reinterpret_cast<unsigned short*>(dest) = *reinterpret_cast<unsigned short*>(src);
	*(dest + 2) = *(src + 2);
}

// SwapAとSwapBを交換(3bytes)

inline void
Swap3bytes(unsigned char* SwapA, unsigned char* SwapB)
{
	unsigned char	Work[3];

	Memcpy3bytes(Work, SwapA);
	Memcpy3bytes(SwapA, SwapB);
	Memcpy3bytes(SwapB, Work);
}

// Int24をInt32に変換

inline int
Int24ToInt32(unsigned char* Num)
{
	return *Num | (*(Num + 1) << 8) | (*reinterpret_cast<char*>(Num + 2) << 16);
}

/*
// string2からcount分、string1にコピー

inline char*
Strncpy(char* string1, const char* string2, size_t count)
{
	size_t	CopyLen = Min(strlen(string2), count);

	memcpy(string1, string2, CopyLen);
	*(string1 + CopyLen) = '\0';

	return string1;
}
*/

