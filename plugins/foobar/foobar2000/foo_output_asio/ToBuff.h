
typedef	void	(*TO_BUFF_ZERO_FUNC)(int Idx);
typedef	void	(*TO_BUFF_FUNC)(int Idx, unsigned char* DataPnt);

void	ToBuff_Zero_Int16LSB(int Idx);
inline void	ToBuff_Float_Int16LSB(double Data, unsigned char* WriteSamplePnt);
void	ToBuff_Int8_Int16LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int16_Int16LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int24_Int16LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int32_Int16LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float32_Int16LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float64_Int16LSB(int Idx, unsigned char* DataPnt);

void	ToBuff_Zero_Int24LSB(int Idx);
inline void	ToBuff_Float_Int24LSB(double Data, unsigned char* WriteSamplePnt);
void	ToBuff_Int8_Int24LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int16_Int24LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int24_Int24LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int32_Int24LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float32_Int24LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float64_Int24LSB(int Idx, unsigned char* DataPnt);

void	ToBuff_Zero_Int32LSB(int Idx);
inline void	ToBuff_Float_Int32LSB(double Data, unsigned char* WriteSamplePnt);
void	ToBuff_Int8_Int32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int16_Int32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int24_Int32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int32_Int32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float32_Int32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float64_Int32LSB(int Idx, unsigned char* DataPnt);

void	ToBuff_Zero_Float32LSB(int Idx);
inline void	ToBuff_Float_Float32(float Data, unsigned char* WriteSamplePnt);
void	ToBuff_Int8_Float32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int16_Float32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int24_Float32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int32_Float32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float32_Float32LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float64_Float32LSB(int Idx, unsigned char* DataPnt);

void	ToBuff_Zero_Float64LSB(int Idx);
inline void	ToBuff_Float_Float64(double Data, unsigned char* WriteSamplePnt);
void	ToBuff_Int8_Float64LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int16_Float64LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int24_Float64LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Int32_Float64LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float32_Float64LSB(int Idx, unsigned char* DataPnt);
void	ToBuff_Float64_Float64LSB(int Idx, unsigned char* DataPnt);

void	ToBuff_Zero_Int32LSB16(int Idx);
inline void	ToBuff_Float_Int32LSB16(double Data, unsigned char* WriteSamplePnt);
void	ToBuff_Int8_Int32LSB16(int Idx, unsigned char* DataPnt);
void	ToBuff_Int16_Int32LSB16(int Idx, unsigned char* DataPnt);
void	ToBuff_Int24_Int32LSB16(int Idx, unsigned char* DataPnt);
void	ToBuff_Int32_Int32LSB16(int Idx, unsigned char* DataPnt);
void	ToBuff_Float32_Int32LSB16(int Idx, unsigned char* DataPnt);
void	ToBuff_Float64_Int32LSB16(int Idx, unsigned char* DataPnt);

void	ToBuff_Zero_Int32LSB24(int Idx);
inline void	ToBuff_Float_Int32LSB24(double Data, unsigned char* WriteSamplePnt);
void	ToBuff_Int8_Int32LSB24(int Idx, unsigned char* DataPnt);
void	ToBuff_Int16_Int32LSB24(int Idx, unsigned char* DataPnt);
void	ToBuff_Int24_Int32LSB24(int Idx, unsigned char* DataPnt);
void	ToBuff_Int32_Int32LSB24(int Idx, unsigned char* DataPnt);
void	ToBuff_Float32_Int32LSB24(int Idx, unsigned char* DataPnt);
void	ToBuff_Float64_Int32LSB24(int Idx, unsigned char* DataPnt);

