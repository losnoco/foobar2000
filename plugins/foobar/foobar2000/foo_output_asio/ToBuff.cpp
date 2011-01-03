
// --------------------------------
//
// Int16
//

void
ToBuff_Zero_Int16LSB(int Idx)
{
	const int	Bps_b = 2;

	*reinterpret_cast<unsigned short*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) = 0;
}

inline void
ToBuff_Float_Int16LSB(double Data, unsigned char* WriteSamplePnt)
{
	int		v;

	_asm {
		fld   qword ptr [Data]
		fistp dword ptr [v]
	}

	if(v > 0x7fff) {
		v = 0x7fff;
	} else if(v < -0x8000) {
		v = -0x8000;
	}

	*reinterpret_cast<short*>(WriteSamplePnt) = v;
}

//
// Int16 (Int 8bit -> Int 16bit)
//

void
ToBuff_Int8_Int16LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 2;
	const int	Shift = 8;

	*reinterpret_cast<short*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									(*DataPnt - 128) << Shift;
}

//
// Int16 (Int 16bit -> Int 16bit)
//

void
ToBuff_Int16_Int16LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 2;

	*reinterpret_cast<unsigned short*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									*reinterpret_cast<unsigned short*>(DataPnt);
}

//
// Int16 (Int 24bit -> Int 16bit)
//

void
ToBuff_Int24_Int16LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 2;
	const int	Shift = 8;

	ToBuff_Float_Int16LSB(
			static_cast<double>(Int24ToInt32(DataPnt)) / (1 << Shift),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int16 (Int 32bit -> Int 16bit)
//

void
ToBuff_Int32_Int16LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 2;
	const int	Shift = 16;

	ToBuff_Float_Int16LSB(
			static_cast<double>(*reinterpret_cast<int*>(DataPnt)) / (1 << Shift),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int16 (Float 32bit -> Int 16bit)
//

void
ToBuff_Float32_Int16LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 2;

	ToBuff_Float_Int16LSB(
			*reinterpret_cast<float*>(DataPnt) * FSCALER16,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int16 (Float 64bit -> Int 16bit)
//

void
ToBuff_Float64_Int16LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 2;

	ToBuff_Float_Int16LSB(
			*reinterpret_cast<double*>(DataPnt) * FSCALER16,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

// --------------------------------
//
// Int24
//

void
ToBuff_Zero_Int24LSB(int Idx)
{
	const int	Bps_b = 3;
	unsigned char*	WriteSamplePnt = ChannelInfos[Idx].Buff + BuffEnd * Bps_b;

	*reinterpret_cast<unsigned short*>(WriteSamplePnt) = 0;
	*(WriteSamplePnt + 2) = 0;
}

inline void
ToBuff_Float_Int24LSB(double Data, unsigned char* WriteSamplePnt)
{
	__int64	v;

	_asm {
		fld   qword ptr [Data]
		fistp qword ptr [v]
	}

	if(v > 0x7fffffi64) {
		v = 0x7fffffi64;
	} else if(v < -0x800000i64) {
		v = -0x800000i64;
	}

	*reinterpret_cast<unsigned short*>(WriteSamplePnt) =
											*reinterpret_cast<unsigned short*>(&v);
	*(WriteSamplePnt + 2) = *(reinterpret_cast<unsigned char*>(&v) + 2);
}

//
// Int24 (Int 8bit -> Int 24bit)
//

void
ToBuff_Int8_Int24LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 3;
	unsigned char*	WriteSamplePnt = ChannelInfos[Idx].Buff + BuffEnd * Bps_b;

	*reinterpret_cast<unsigned short*>(WriteSamplePnt) = 0;
	*(WriteSamplePnt + 2) = *DataPnt - 128;
}

//
// Int24 (Int 16bit -> Int 24bit)
//

void
ToBuff_Int16_Int24LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 3;
	unsigned char*	WriteSamplePnt = ChannelInfos[Idx].Buff + BuffEnd * Bps_b;

	*WriteSamplePnt = 0;
	*reinterpret_cast<unsigned short*>(WriteSamplePnt + 1) =
									*reinterpret_cast<unsigned short*>(DataPnt);
}

//
// Int24 (Int 24bit -> Int 24bit)
//

void
ToBuff_Int24_Int24LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 3;

	Memcpy3bytes(ChannelInfos[Idx].Buff + BuffEnd * Bps_b, DataPnt);
}

//
// Int24 (Int 32bit -> Int 24bit)
//

void
ToBuff_Int32_Int24LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 3;
	const int	Shift = 8;

	ToBuff_Float_Int24LSB(
			static_cast<double>(*reinterpret_cast<int*>(DataPnt)) / (1 << Shift),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int24 (Float 32bit -> Int 24bit)
//

void
ToBuff_Float32_Int24LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 3;

	ToBuff_Float_Int24LSB(
			*reinterpret_cast<float*>(DataPnt) * FSCALER24,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int24 (Float 64bit -> Int 24bit)
//

void
ToBuff_Float64_Int24LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 3;

	ToBuff_Float_Int24LSB(
			*reinterpret_cast<double*>(DataPnt) * FSCALER24,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

// --------------------------------
//
// Int32
//

void
ToBuff_Zero_Int32LSB(int Idx)
{
	const int	Bps_b = 4;

	*reinterpret_cast<unsigned int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) = 0;
}

inline void
ToBuff_Float_Int32LSB(double Data, unsigned char* WriteSamplePnt)
{
	__int64	v;

	_asm {
		fld   qword ptr [Data]
		fistp qword ptr [v]
	}

	if(v > 0x7fffffffi64) {
		v = 0x7fffffffi64;
	} else if(v < -0x80000000i64) {
		v = -0x80000000i64;
	}

	*reinterpret_cast<int*>(WriteSamplePnt) = static_cast<int>(v);
}

//
// Int32 (Int 8bit -> Int 32bit)
//

void
ToBuff_Int8_Int32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;
	const int	Shift = 24;

	*reinterpret_cast<int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									(*DataPnt - 128) << Shift;
}

//
// Int32 (Int 16bit -> Int 32bit)
//

void
ToBuff_Int16_Int32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;
	const int	Shift = 16;

	*reinterpret_cast<int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									*reinterpret_cast<short*>(DataPnt) << Shift;
}

//
// Int32 (Int 24bit -> Int 32bit)
//

void
ToBuff_Int24_Int32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;
	const int	Shift = 8;

	*reinterpret_cast<int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									Int24ToInt32(DataPnt) << Shift;
}

//
// Int32 (Int 32bit -> Int 32bit)
//

void
ToBuff_Int32_Int32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	*reinterpret_cast<unsigned int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									*reinterpret_cast<unsigned int*>(DataPnt);
}

//
// Int32 (Float 32bit -> Int 32bit)
//

void
ToBuff_Float32_Int32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Int32LSB(
			*reinterpret_cast<float*>(DataPnt) * FSCALER32,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int32 (Float 64bit -> Int 32bit)
//

void
ToBuff_Float64_Int32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Int32LSB(
			*reinterpret_cast<double*>(DataPnt) * FSCALER32,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

// --------------------------------
//
// Float32
//

void
ToBuff_Zero_Float32LSB(int Idx)
{
	const int	Bps_b = 4;

	*reinterpret_cast<float*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) = 0.0;
}

inline void
ToBuff_Float_Float32(float Data, unsigned char* WriteSamplePnt)
{
	if(Data > 1.0) {
		Data = 1.0;
	} else if(Data < -1.0) {
		Data = -1.0;
	}

	*reinterpret_cast<float*>(WriteSamplePnt) = Data;
}

//
// Float32 (Int 8bit -> Float 32bit)
//

void
ToBuff_Int8_Float32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Float32(
			static_cast<float>((*DataPnt - 128) / FSCALER8),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float32 (Int 16bit -> Float 32bit)
//

void
ToBuff_Int16_Float32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Float32(
			static_cast<float>(*reinterpret_cast<short*>(DataPnt) / FSCALER16),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float32 (Int 24bit -> Float 32bit)
//

void
ToBuff_Int24_Float32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Float32(
			static_cast<float>(Int24ToInt32(DataPnt) / FSCALER24),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float32 (Int 32bit -> Float 32bit)
//

void
ToBuff_Int32_Float32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Float32(
			static_cast<float>(*reinterpret_cast<int*>(DataPnt) / FSCALER32),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float32 (Float 32bit -> Float 32bit)
//

void
ToBuff_Float32_Float32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Float32(
			*reinterpret_cast<float*>(DataPnt),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float32 (Float 64bit -> Float 32bit)
//

void
ToBuff_Float64_Float32LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Float32(
			static_cast<float>(*reinterpret_cast<double*>(DataPnt)),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

// --------------------------------
//
// Float64
//

void
ToBuff_Zero_Float64LSB(int Idx)
{
	const int	Bps_b = 8;

	*reinterpret_cast<double*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) = 0.0;
}

inline void
ToBuff_Float_Float64(double Data, unsigned char* WriteSamplePnt)
{
	if(Data > 1.0) {
		Data = 1.0;
	} else if(Data < -1.0) {
		Data = -1.0;
	}

	*reinterpret_cast<double*>(WriteSamplePnt) = Data;
}

//
// Float64 (Int 8bit -> Float 64bit)
//

void
ToBuff_Int8_Float64LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 8;

	ToBuff_Float_Float64(
			(*DataPnt - 128) / FSCALER8,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float64 (Int 16bit -> Float 64bit)
//

void
ToBuff_Int16_Float64LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 8;

	ToBuff_Float_Float64(
			*reinterpret_cast<short*>(DataPnt) / FSCALER16,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float64 (Int 24bit -> Float 64bit)
//

void
ToBuff_Int24_Float64LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 8;

	ToBuff_Float_Float64(
			Int24ToInt32(DataPnt) / FSCALER24,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float64 (Int 32bit -> Float 64bit)
//

void
ToBuff_Int32_Float64LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 8;

	ToBuff_Float_Float64(
			*reinterpret_cast<int*>(DataPnt) / FSCALER32,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float64 (Float 32bit -> Float 64bit)
//

void
ToBuff_Float32_Float64LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 8;

	ToBuff_Float_Float64(
			*reinterpret_cast<float*>(DataPnt),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Float64 (Float 64bit -> Float 64bit)
//

void
ToBuff_Float64_Float64LSB(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 8;

	ToBuff_Float_Float64(
			*reinterpret_cast<double*>(DataPnt),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

// --------------------------------
//
// Int16 in Int32
//

void
ToBuff_Zero_Int32LSB16(int Idx)
{
	const int	Bps_b = 4;

	*reinterpret_cast<unsigned int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) = 0;
}

inline void
ToBuff_Float_Int32LSB16(double Data, unsigned char* WriteSamplePnt)
{
	int		v;

	_asm {
		fld   qword ptr [Data]
		fistp dword ptr [v]
	}

	if(v > 0x7fff) {
		v = 0x7fff;
	} else if(v < -0x8000) {
		v = -0x8000;
	}

	*reinterpret_cast<int*>(WriteSamplePnt) = v;
}

//
// Int16 in Int32 (Int 8bit -> Int 16bit)
//

void
ToBuff_Int8_Int32LSB16(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;
	const int	Shift = 8;

	*reinterpret_cast<int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									(*DataPnt - 128) << Shift;
}

//
// Int16 in Int32 (Int 16bit -> Int 16bit)
//

void
ToBuff_Int16_Int32LSB16(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	*reinterpret_cast<int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									*reinterpret_cast<short*>(DataPnt);
}

//
// Int16 in Int32 (Int 24bit -> Int 16bit)
//

void
ToBuff_Int24_Int32LSB16(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;
	const int	Shift = 8;

	ToBuff_Float_Int32LSB16(
			static_cast<double>(Int24ToInt32(DataPnt)) / (1 << Shift),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int16 in Int32 (Int 32bit -> Int 16bit)
//

void
ToBuff_Int32_Int32LSB16(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;
	const int	Shift = 16;

	ToBuff_Float_Int32LSB16(
			static_cast<double>(*reinterpret_cast<int*>(DataPnt)) / (1 << Shift),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int16 in Int32 (Float 32bit -> Int 16bit)
//

void
ToBuff_Float32_Int32LSB16(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Int32LSB16(
			*reinterpret_cast<float*>(DataPnt) * FSCALER16,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int16 in Int32 (Float 64bit -> Int 16bit)
//

void
ToBuff_Float64_Int32LSB16(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Int32LSB16(
			*reinterpret_cast<double*>(DataPnt) * FSCALER16,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

// --------------------------------
//
// Int24 in Int32
//

void
ToBuff_Zero_Int32LSB24(int Idx)
{
	const int	Bps_b = 4;

	*reinterpret_cast<unsigned int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) = 0;
}

inline void
ToBuff_Float_Int32LSB24(double Data, unsigned char* WriteSamplePnt)
{
	__int64	v;

	_asm {
		fld   qword ptr [Data]
		fistp qword ptr [v]
	}

	if(v > 0x7fffffi64) {
		v = 0x7fffffi64;
	} else if(v < -0x800000i64) {
		v = -0x800000i64;
	}

	*reinterpret_cast<int*>(WriteSamplePnt) = static_cast<int>(v);
}

//
// Int24 in Int32 (Int 8bit -> Int 24bit)
//

void
ToBuff_Int8_Int32LSB24(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;
	const int	Shift = 16;

	*reinterpret_cast<int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									(*DataPnt - 128) << Shift;
}

//
// Int24 in Int32 (Int 16bit -> Int 24bit)
//

void
ToBuff_Int16_Int32LSB24(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;
	const int	Shift = 8;

	*reinterpret_cast<int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									*reinterpret_cast<short*>(DataPnt) << Shift;
}

//
// Int24 in Int32 (Int 24bit -> Int 24bit)
//

void
ToBuff_Int24_Int32LSB24(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	*reinterpret_cast<int*>(ChannelInfos[Idx].Buff + BuffEnd * Bps_b) =
									Int24ToInt32(DataPnt);
}

//
// Int24 in Int32 (Int 32bit -> Int 24bit)
//

void
ToBuff_Int32_Int32LSB24(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;
	const int	Shift = 8;

	ToBuff_Float_Int32LSB24(
			static_cast<double>(*reinterpret_cast<int*>(DataPnt)) / (1 << Shift),
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int24 in Int32 (Float 32bit -> Int 24bit)
//

void
ToBuff_Float32_Int32LSB24(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Int32LSB24(
			*reinterpret_cast<float*>(DataPnt) * FSCALER24,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

//
// Int24 in Int32 (Float 64bit -> Int 24bit)
//

void
ToBuff_Float64_Int32LSB24(int Idx, unsigned char* DataPnt)
{
	const int	Bps_b = 4;

	ToBuff_Float_Int32LSB24(
			*reinterpret_cast<double*>(DataPnt) * FSCALER24,
			ChannelInfos[Idx].Buff + BuffEnd * Bps_b);
}

