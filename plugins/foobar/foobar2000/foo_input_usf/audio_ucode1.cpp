#include "usf.h"
#include "audio_hle.h"


void Abi1::SPNOOP()
{

}


uint16_t ResampleLUT [0x200] =
{
	0x0C39, 0x66AD, 0x0D46, 0xFFDF, 0x0B39, 0x6696, 0x0E5F, 0xFFD8,
	0x0A44, 0x6669, 0x0F83, 0xFFD0, 0x095A, 0x6626, 0x10B4, 0xFFC8,
	0x087D, 0x65CD, 0x11F0, 0xFFBF, 0x07AB, 0x655E, 0x1338, 0xFFB6,
	0x06E4, 0x64D9, 0x148C, 0xFFAC, 0x0628, 0x643F, 0x15EB, 0xFFA1,
	0x0577, 0x638F, 0x1756, 0xFF96, 0x04D1, 0x62CB, 0x18CB, 0xFF8A,
	0x0435, 0x61F3, 0x1A4C, 0xFF7E, 0x03A4, 0x6106, 0x1BD7, 0xFF71,
	0x031C, 0x6007, 0x1D6C, 0xFF64, 0x029F, 0x5EF5, 0x1F0B, 0xFF56,
	0x022A, 0x5DD0, 0x20B3, 0xFF48, 0x01BE, 0x5C9A, 0x2264, 0xFF3A,
	0x015B, 0x5B53, 0x241E, 0xFF2C, 0x0101, 0x59FC, 0x25E0, 0xFF1E,
	0x00AE, 0x5896, 0x27A9, 0xFF10, 0x0063, 0x5720, 0x297A, 0xFF02,
	0x001F, 0x559D, 0x2B50, 0xFEF4, 0xFFE2, 0x540D, 0x2D2C, 0xFEE8,
	0xFFAC, 0x5270, 0x2F0D, 0xFEDB, 0xFF7C, 0x50C7, 0x30F3, 0xFED0,
	0xFF53, 0x4F14, 0x32DC, 0xFEC6, 0xFF2E, 0x4D57, 0x34C8, 0xFEBD,
	0xFF0F, 0x4B91, 0x36B6, 0xFEB6, 0xFEF5, 0x49C2, 0x38A5, 0xFEB0,
	0xFEDF, 0x47ED, 0x3A95, 0xFEAC, 0xFECE, 0x4611, 0x3C85, 0xFEAB,
	0xFEC0, 0x4430, 0x3E74, 0xFEAC, 0xFEB6, 0x424A, 0x4060, 0xFEAF,
	0xFEAF, 0x4060, 0x424A, 0xFEB6, 0xFEAC, 0x3E74, 0x4430, 0xFEC0,
	0xFEAB, 0x3C85, 0x4611, 0xFECE, 0xFEAC, 0x3A95, 0x47ED, 0xFEDF,
	0xFEB0, 0x38A5, 0x49C2, 0xFEF5, 0xFEB6, 0x36B6, 0x4B91, 0xFF0F,
	0xFEBD, 0x34C8, 0x4D57, 0xFF2E, 0xFEC6, 0x32DC, 0x4F14, 0xFF53,
	0xFED0, 0x30F3, 0x50C7, 0xFF7C, 0xFEDB, 0x2F0D, 0x5270, 0xFFAC,
	0xFEE8, 0x2D2C, 0x540D, 0xFFE2, 0xFEF4, 0x2B50, 0x559D, 0x001F,
	0xFF02, 0x297A, 0x5720, 0x0063, 0xFF10, 0x27A9, 0x5896, 0x00AE,
	0xFF1E, 0x25E0, 0x59FC, 0x0101, 0xFF2C, 0x241E, 0x5B53, 0x015B,
	0xFF3A, 0x2264, 0x5C9A, 0x01BE, 0xFF48, 0x20B3, 0x5DD0, 0x022A,
	0xFF56, 0x1F0B, 0x5EF5, 0x029F, 0xFF64, 0x1D6C, 0x6007, 0x031C,
	0xFF71, 0x1BD7, 0x6106, 0x03A4, 0xFF7E, 0x1A4C, 0x61F3, 0x0435,
	0xFF8A, 0x18CB, 0x62CB, 0x04D1, 0xFF96, 0x1756, 0x638F, 0x0577,
	0xFFA1, 0x15EB, 0x643F, 0x0628, 0xFFAC, 0x148C, 0x64D9, 0x06E4,
	0xFFB6, 0x1338, 0x655E, 0x07AB, 0xFFBF, 0x11F0, 0x65CD, 0x087D,
	0xFFC8, 0x10B4, 0x6626, 0x095A, 0xFFD0, 0x0F83, 0x6669, 0x0A44,
	0xFFD8, 0x0E5F, 0x6696, 0x0B39, 0xFFDF, 0x0D46, 0x66AD, 0x0C39
};



void Abi1::CLEARBUFF ()
{
	uint32_t addr = (uint32_t)(inst1 & 0xffff);
	uint32_t count = (uint32_t)(inst2 & 0xffff);
	addr &= 0xFFFC;
	memset(BufferSpace+addr, 0, (count+3)&0xFFFC);
}


void Abi1::ENVMIXER () {

	uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
	uint32_t addy = (inst2 & 0xFFFFFF);
	short *inp=(short *)(BufferSpace+AudioInBuffer);
	short *out=(short *)(BufferSpace+AudioOutBuffer);
	short *aux1=(short *)(BufferSpace+AudioAuxA);
	short *aux2=(short *)(BufferSpace+AudioAuxC);
	short *aux3=(short *)(BufferSpace+AudioAuxE);
	int32_t MainR;
	int32_t MainL;
	int32_t AuxR;
	int32_t AuxL;
	int i1,o1,a1,a2=0,a3=0;
	unsigned short AuxIncRate=1;
	short zero[8];
	int32_t LVol, RVol;
	int32_t LAcc, RAcc;
	int32_t LTrg, RTrg;
	int16_t Wet, Dry;
	uint32_t ptr = 0;
	int32_t RRamp, LRamp;
	int32_t LAdderStart, RAdderStart, LAdderEnd, RAdderEnd;
	int32_t oMainR, oMainL, oAuxR, oAuxL;
	int x = 0, y = 0;

	memset(zero,0,16);

	if (flags & A_INIT) {

		if(Goldeneye) {
			//Vol_Left = Vol_Right;
			// VolTrg_Left = VolTrg_Right;
			Vol_Left = (Vol_Right >> 8) << 8;
			VolTrg_Left = (VolTrg_Right >> 8) << 8;
			//Vol_Right = (Vol_Right & 0xff) << 8;
			//VolTrg_Right = (VolTrg_Right & 0xff) << 8;
		}

		LVol = ((Vol_Left * (int32_t)VolRamp_Left));
		RVol = ((Vol_Right * (int32_t)VolRamp_Right));

		Wet = (int16_t)Env_Wet;
		Dry = (int16_t)Env_Dry; // Save Wet/Dry values
		LTrg = (VolTrg_Left << 16);
		RTrg = (VolTrg_Right << 16); // Save Current Left/Right Targets
		LAdderStart = Vol_Left << 16;
		RAdderStart = Vol_Right << 16;
		LAdderEnd = LVol ;
		RAdderEnd = RVol ;
		RRamp = VolRamp_Right;
		LRamp = VolRamp_Left;
	} else {
		// Load LVol, RVol, LAcc, and RAcc (all 32bit)
		// Load Wet, Dry, LTrg, RTrg
		memcpy((uint8_t*)hleMixerWorkArea, (uint8_t*)hCpu.N64MEM+addy, 80);
		Wet  = *(int16_t *)(hleMixerWorkArea +  0); // 0-1
		Dry  = *(int16_t *)(hleMixerWorkArea +  2); // 2-3
		LTrg = *(int32_t *)(hleMixerWorkArea +  4); // 4-5
		RTrg = *(int32_t *)(hleMixerWorkArea +  6); // 6-7
		LRamp= *(int32_t *)(hleMixerWorkArea +  8); // 8-9 (hleMixerWorkArea is a 16bit pointer)
		RRamp= *(int32_t *)(hleMixerWorkArea + 10); // 10-11
		LAdderEnd = *(int32_t *)(hleMixerWorkArea + 12); // 12-13
		RAdderEnd = *(int32_t *)(hleMixerWorkArea + 14); // 14-15
		LAdderStart = *(int32_t *)(hleMixerWorkArea + 16); // 12-13
		RAdderStart = *(int32_t *)(hleMixerWorkArea + 18); // 14-15
	}

	if(!(flags&A_AUX)) {
		AuxIncRate=0;
		aux2=aux3=zero;
	}

	oMainL = (Dry * (LTrg>>16) + 0x4000) >> 15;
	oAuxL  = (Wet * (LTrg>>16) + 0x4000)  >> 15;
	oMainR = (Dry * (RTrg>>16) + 0x4000) >> 15;
	oAuxR  = (Wet * (RTrg>>16) + 0x4000)  >> 15;

	for (y = 0; y < AudioCount; y += 0x10) {

		if (LAdderStart != LTrg) {
			LAcc = LAdderStart;
			LVol = (LAdderEnd - LAdderStart) >> 3;
			LAdderEnd   = ((int64_t)LAdderEnd * (int64_t)LRamp) >> 16;
			LAdderStart = ((int64_t)LAcc * (int64_t)LRamp) >> 16;
		} else {
			LAcc = LTrg;
			LVol = 0;
		}

		if (RAdderStart != RTrg) {
			RAcc = RAdderStart;
			RVol = (RAdderEnd - RAdderStart) >> 3;
			RAdderEnd   = ((int64_t)RAdderEnd * (int64_t)RRamp) >> 16;
			RAdderStart = ((int64_t)RAcc * (int64_t)RRamp) >> 16;
		} else {
			RAcc = RTrg;
			RVol = 0;
		}

	for (x = 0; x < 8; x++) {
		i1=(int)inp[ptr^1];
		o1=(int)out[ptr^1];
		a1=(int)aux1[ptr^1];
		if (AuxIncRate) {
			a2=(int)aux2[ptr^1];
			a3=(int)aux3[ptr^1];
		}

		LAcc += LVol;
		RAcc += RVol;

		if (LVol <= 0) { // Decrementing
			if (LAcc < LTrg) {
				LAcc = LTrg;
				LAdderStart = LTrg;
				MainL = oMainL;
				AuxL  = oAuxL;
			} else {
				MainL = (Dry * ((int32_t)LAcc>>16) + 0x4000) >> 15;
				AuxL  = (Wet * ((int32_t)LAcc>>16) + 0x4000)  >> 15;
			}
		} else {
			if (LAcc > LTrg) {
				LAcc = LTrg;
				LAdderStart = LTrg;
				MainL = oMainL;
				AuxL  = oAuxL;
			} else {
				MainL = (Dry * ((int32_t)LAcc>>16) + 0x4000) >> 15;
				AuxL  = (Wet * ((int32_t)LAcc>>16) + 0x4000)  >> 15;
			}
		}

		if (RVol <= 0) { // Decrementing
			if (RAcc < RTrg) {
				RAcc = RTrg;
				RAdderStart = RTrg;
				MainR = oMainR;
				AuxR  = oAuxR;
			} else {
				MainR = (Dry * ((int32_t)RAcc>>16) + 0x4000) >> 15;
				AuxR  = (Wet * ((int32_t)RAcc>>16) + 0x4000)  >> 15;
			}
		} else {
			if (RAcc > RTrg) {
				RAcc = RTrg;
				RAdderStart = RTrg;
				MainR = oMainR;
				AuxR  = oAuxR;
			} else {
				MainR = (Dry * ((int32_t)RAcc>>16) + 0x4000) >> 15;
				AuxR  = (Wet * ((int32_t)RAcc>>16) + 0x4000)  >> 15;
			}
		}

		o1+=((i1*MainR)+0x4000)>>15;
		a1+=((i1*MainL)+0x4000)>>15;

		if(o1>32767) o1=32767;
		else if(o1<-32768) o1=-32768;

		if(a1>32767) a1=32767;
		else if(a1<-32768) a1=-32768;

		out[ptr^1]=o1;
		aux1[ptr^1]=a1;
		if (AuxIncRate) {
			a2+=((i1*AuxR)+0x4000)>>15;
			a3+=((i1*AuxL)+0x4000)>>15;

			if(a2>32767) a2=32767;
			else if(a2<-32768) a2=-32768;

			if(a3>32767) a3=32767;
			else if(a3<-32768) a3=-32768;

			aux2[ptr^1]=a2;
			aux3[ptr^1]=a3;
		}
		ptr++;
	}
	}

	*(int16_t *)(hleMixerWorkArea +  0) = Wet; // 0-1
	*(int16_t *)(hleMixerWorkArea +  2) = Dry; // 2-3
	*(int32_t *)(hleMixerWorkArea +  4) = LTrg; // 4-5
	*(int32_t *)(hleMixerWorkArea +  6) = RTrg; // 6-7
	*(int32_t *)(hleMixerWorkArea +  8) = LRamp; // 8-9 (hleMixerWorkArea is a 16bit pointer)
	*(int32_t *)(hleMixerWorkArea + 10) = RRamp; // 10-11
	*(int32_t *)(hleMixerWorkArea + 12) = LAdderEnd; // 12-13
	*(int32_t *)(hleMixerWorkArea + 14) = RAdderEnd; // 14-15
	*(int32_t *)(hleMixerWorkArea + 16) = LAdderStart; // 12-13
	*(int32_t *)(hleMixerWorkArea + 18) = RAdderStart; // 14-15

	memcpy((uint8_t*)hCpu.N64MEM+addy,(uint8_t*)hleMixerWorkArea,80);
}

void Abi1::RESAMPLE () {
	unsigned int Accum=0;
	unsigned int location;
	int16_t *lut/*, *lut2*/;
	short *dst;
	int16_t *src;
	int32_t temp;
	int32_t accum,addy;
	uint32_t srcPtr, dstPtr;
	unsigned char Flags;
	unsigned int Pitch;
	int x = 0, i = 0;

	dst=(short *)(BufferSpace);
	src=(int16_t *)(BufferSpace);
	srcPtr=(AudioInBuffer/2);
	dstPtr=(AudioOutBuffer/2);


	addy = (inst2 & 0xffffff);// + SEGMENTS[(inst2>>24)&0xf];
	Flags=(uint8_t)((inst1>>16)&0xff);
	Pitch=((inst1&0xffff))<<1;

	srcPtr -= 4;

	if ((Flags & 0x1) == 0) {
		for (x=0; x < 4; x++)
			src[(srcPtr+x)^1] = *(uint16_t *)(hCpu.N64MEM+(addy+(x*2)));

		Accum = *(uint16_t *)(hCpu.N64MEM+addy+10);
	} else {
		for (x=0; x < 4; x++)
			src[(srcPtr+x)^1] = 0;
	}


	for(i = 0; i < ((AudioCount+0xf)&0xFFF0)/2;i++)	{

		location = (Accum >> 0xa) << 0x3;
		lut = (int16_t *)(((uint8_t *)ResampleLUT) + location);

		temp =  ((int32_t)*(int16_t*)(src+((srcPtr+0)^1))*((int32_t)((int16_t)lut[0])));
		accum = (int32_t)(temp >> 15);

		temp = ((int32_t)*(int16_t*)(src+((srcPtr+1)^1))*((int32_t)((int16_t)lut[1])));
		accum += (int32_t)(temp >> 15);

		temp = ((int32_t)*(int16_t*)(src+((srcPtr+2)^1))*((int32_t)((int16_t)lut[2])));
		accum += (int32_t)(temp >> 15);

		temp = ((int32_t)*(int16_t*)(src+((srcPtr+3)^1))*((int32_t)((int16_t)lut[3])));
		accum += (int32_t)(temp >> 15);

		if (accum > 32767) accum = 32767;
		if (accum < -32768) accum = -32768;

		dst[dstPtr^1] = (accum);
		dstPtr++;
		Accum += Pitch;
		srcPtr += (Accum>>16);
		Accum&=0xffff;
	}
	for (x = 0; x < 4; x++)
		*(uint16_t *)(hCpu.N64MEM+(addy+(x*2))) = src[(srcPtr+x)^1];

	*(uint16_t *)(hCpu.N64MEM+addy+10) = Accum;
}
void Abi1::SETVOL () {
// Might be better to unpack these depending on the flags...
	uint8_t flags = (uint8_t)((inst1 >> 16) & 0xff);
	uint16_t vol = (int16_t)(inst1 & 0xffff);
	uint16_t voltarg =(uint16_t)((inst2 >> 16)&0xffff);
	uint16_t volrate = (uint16_t)((inst2 & 0xffff));

	if (flags & A_AUX) {
		Env_Dry = (int16_t)vol;			// m_MainVol
		Env_Wet = (int16_t)volrate;		// m_AuxVol
		return;
	}

	if(flags & A_VOL) { // Set the Source(start) Volumes
		if(flags & A_LEFT) {
			Vol_Left = (int16_t)vol;	// m_LeftVolume
		} else { // A_RIGHT
			Vol_Right = (int16_t)vol;	// m_RightVolume
		}
		return;
	}

	if(flags & A_LEFT) { // Set the Ramping values Target, Ramp
		VolTrg_Left  = *(int16_t *)&inst1;		// m_LeftVol
		if(Goldeneye)
			VolRamp_Left = voltarg;
		else
			VolRamp_Left = (int32_t)volrate;//*(int32_t *)&inst2;//(uint16_t)(inst2) | (int32_t)(int16_t)(inst2 << 0x10);
	} else { // A_RIGHT
		VolTrg_Right  = *(int16_t *)&inst1;		// m_RightVol
		if(Goldeneye)
			VolRamp_Right = voltarg;
		else
			VolRamp_Right = (int32_t)volrate;//*(int32_t *)&inst2;//(uint16_t)(inst2 >> 0x10) | (int32_t)(int16_t)(inst2 << 0x10);
	}
}

void Abi1::UNKNOWN () {}

void Abi1::SETLOOP () {
	loopval = (inst2 & 0xffffff);
}

void Abi1::ADPCM () { // Work in progress! :)
	unsigned short inPtr=0;
	unsigned char icode;
	unsigned char code;
	int vscale;
	unsigned short index;
	unsigned short j;
	int a[8];
	short *book1,*book2;
	int l1,l2;
	int inp1[8];
	int inp2[8];
	short *out, count;

	unsigned char Flags=(uint8_t)(inst1>>16)&0xff;
	unsigned int Address=(inst2 & 0xffffff);

	out=(short *)(BufferSpace+AudioOutBuffer);
	count=(short)AudioCount;

	memset(out,0,32);

	if(!(Flags&0x1))
	{
		if(Flags&0x2) {
			memcpy((uint8_t*)out,(uint8_t*)hCpu.N64MEM+Address,32);
		} else {
			memcpy((uint8_t*)out,(uint8_t*)hCpu.N64MEM+Address,32);
		}
	}

	l1=out[15];
	l2=out[14];
	out+=16;
	while(count>0)
	{
													// the first interation through, these values are
													// either 0 in the case of A_INIT, from a special
													// area of memory in the case of A_LOOP or just
													// the values we calculated the last time

		code=BufferSpace[(AudioInBuffer+inPtr)^3];
		index=code&0xf;
		index<<=4;									// index into the adpcm code table
		book1=(short *)&adpcmtable[index];
		book2=book1+8;
		code>>=4;									// upper nibble is scale
		vscale=(0x8000>>((12-code)-1));			// very strange. 0x8000 would be .5 in 16:16 format
													// so this appears to be a fractional scale based
													// on the 12 based inverse of the scale value.  note
													// that this could be negative, in which case we do
													// not use the calculated vscale value... see the
													// if(code>12) check below

		inPtr++;									// coded adpcm data lies next
		j=0;
		while(j<8)									// loop of 8, for 8 coded nibbles from 4 bytes
													// which yields 8 short pcm values
		{
			icode=BufferSpace[(AudioInBuffer+inPtr)^3];
			inPtr++;

			inp1[j]=(int16_t)((icode&0xf0)<<8);			// this will in effect be signed
			if(code<12)
				inp1[j]=((int)((int)inp1[j]*(int)vscale)>>16);
			j++;

			inp1[j]=(int16_t)((icode&0xf)<<12);
			if(code<12)
				inp1[j]=((int)((int)inp1[j]*(int)vscale)>>16);
			j++;
		}
		j=0;
		while(j<8)
		{
			icode=BufferSpace[(AudioInBuffer+inPtr)^3];
			inPtr++;

			inp2[j]=(short)((icode&0xf0)<<8);			// this will in effect be signed
			if(code<12)
				inp2[j]=((int)((int)inp2[j]*(int)vscale)>>16);
			j++;

			inp2[j]=(short)((icode&0xf)<<12);
			if(code<12)
				inp2[j]=((int)((int)inp2[j]*(int)vscale)>>16);
			j++;
		}

		a[0]= (int)book1[0]*(int)l1;
		a[0]+=(int)book2[0]*(int)l2;
		a[0]+=(int)inp1[0]*(int)2048;

		a[1] =(int)book1[1]*(int)l1;
		a[1]+=(int)book2[1]*(int)l2;
		a[1]+=(int)book2[0]*inp1[0];
		a[1]+=(int)inp1[1]*(int)2048;

		a[2] =(int)book1[2]*(int)l1;
		a[2]+=(int)book2[2]*(int)l2;
		a[2]+=(int)book2[1]*inp1[0];
		a[2]+=(int)book2[0]*inp1[1];
		a[2]+=(int)inp1[2]*(int)2048;

		a[3] =(int)book1[3]*(int)l1;
		a[3]+=(int)book2[3]*(int)l2;
		a[3]+=(int)book2[2]*inp1[0];
		a[3]+=(int)book2[1]*inp1[1];
		a[3]+=(int)book2[0]*inp1[2];
		a[3]+=(int)inp1[3]*(int)2048;

		a[4] =(int)book1[4]*(int)l1;
		a[4]+=(int)book2[4]*(int)l2;
		a[4]+=(int)book2[3]*inp1[0];
		a[4]+=(int)book2[2]*inp1[1];
		a[4]+=(int)book2[1]*inp1[2];
		a[4]+=(int)book2[0]*inp1[3];
		a[4]+=(int)inp1[4]*(int)2048;

		a[5] =(int)book1[5]*(int)l1;
		a[5]+=(int)book2[5]*(int)l2;
		a[5]+=(int)book2[4]*inp1[0];
		a[5]+=(int)book2[3]*inp1[1];
		a[5]+=(int)book2[2]*inp1[2];
		a[5]+=(int)book2[1]*inp1[3];
		a[5]+=(int)book2[0]*inp1[4];
		a[5]+=(int)inp1[5]*(int)2048;

		a[6] =(int)book1[6]*(int)l1;
		a[6]+=(int)book2[6]*(int)l2;
		a[6]+=(int)book2[5]*inp1[0];
		a[6]+=(int)book2[4]*inp1[1];
		a[6]+=(int)book2[3]*inp1[2];
		a[6]+=(int)book2[2]*inp1[3];
		a[6]+=(int)book2[1]*inp1[4];
		a[6]+=(int)book2[0]*inp1[5];
		a[6]+=(int)inp1[6]*(int)2048;

		a[7] =(int)book1[7]*(int)l1;
		a[7]+=(int)book2[7]*(int)l2;
		a[7]+=(int)book2[6]*inp1[0];
		a[7]+=(int)book2[5]*inp1[1];
		a[7]+=(int)book2[4]*inp1[2];
		a[7]+=(int)book2[3]*inp1[3];
		a[7]+=(int)book2[2]*inp1[4];
		a[7]+=(int)book2[1]*inp1[5];
		a[7]+=(int)book2[0]*inp1[6];
		a[7]+=(int)inp1[7]*(int)2048;

		for(j=0;j<8;j++)
		{
			a[j^1]>>=11;
			if(a[j^1]>32767) a[j^1]=32767;
			else if(a[j^1]<-32768) a[j^1]=-32768;
			*(out++)=a[j^1];
		}
		l1=a[6];
		l2=a[7];

		a[0]= (int)book1[0]*(int)l1;
		a[0]+=(int)book2[0]*(int)l2;
		a[0]+=(int)inp2[0]*(int)2048;

		a[1] =(int)book1[1]*(int)l1;
		a[1]+=(int)book2[1]*(int)l2;
		a[1]+=(int)book2[0]*inp2[0];
		a[1]+=(int)inp2[1]*(int)2048;

		a[2] =(int)book1[2]*(int)l1;
		a[2]+=(int)book2[2]*(int)l2;
		a[2]+=(int)book2[1]*inp2[0];
		a[2]+=(int)book2[0]*inp2[1];
		a[2]+=(int)inp2[2]*(int)2048;

		a[3] =(int)book1[3]*(int)l1;
		a[3]+=(int)book2[3]*(int)l2;
		a[3]+=(int)book2[2]*inp2[0];
		a[3]+=(int)book2[1]*inp2[1];
		a[3]+=(int)book2[0]*inp2[2];
		a[3]+=(int)inp2[3]*(int)2048;

		a[4] =(int)book1[4]*(int)l1;
		a[4]+=(int)book2[4]*(int)l2;
		a[4]+=(int)book2[3]*inp2[0];
		a[4]+=(int)book2[2]*inp2[1];
		a[4]+=(int)book2[1]*inp2[2];
		a[4]+=(int)book2[0]*inp2[3];
		a[4]+=(int)inp2[4]*(int)2048;

		a[5] =(int)book1[5]*(int)l1;
		a[5]+=(int)book2[5]*(int)l2;
		a[5]+=(int)book2[4]*inp2[0];
		a[5]+=(int)book2[3]*inp2[1];
		a[5]+=(int)book2[2]*inp2[2];
		a[5]+=(int)book2[1]*inp2[3];
		a[5]+=(int)book2[0]*inp2[4];
		a[5]+=(int)inp2[5]*(int)2048;

		a[6] =(int)book1[6]*(int)l1;
		a[6]+=(int)book2[6]*(int)l2;
		a[6]+=(int)book2[5]*inp2[0];
		a[6]+=(int)book2[4]*inp2[1];
		a[6]+=(int)book2[3]*inp2[2];
		a[6]+=(int)book2[2]*inp2[3];
		a[6]+=(int)book2[1]*inp2[4];
		a[6]+=(int)book2[0]*inp2[5];
		a[6]+=(int)inp2[6]*(int)2048;

		a[7] =(int)book1[7]*(int)l1;
		a[7]+=(int)book2[7]*(int)l2;
		a[7]+=(int)book2[6]*inp2[0];
		a[7]+=(int)book2[5]*inp2[1];
		a[7]+=(int)book2[4]*inp2[2];
		a[7]+=(int)book2[3]*inp2[3];
		a[7]+=(int)book2[2]*inp2[4];
		a[7]+=(int)book2[1]*inp2[5];
		a[7]+=(int)book2[0]*inp2[6];
		a[7]+=(int)inp2[7]*(int)2048;

		for(j=0;j<8;j++)
		{
			a[j^1]>>=11;
			if(a[j^1]>32767) a[j^1]=32767;
			else if(a[j^1]<-32768) a[j^1]=-32768;
			*(out++)=a[j^1];
		}
		l1=a[6];
		l2=a[7];

		count-=32;
	}
	out-=16;

	memcpy((uint8_t*)hCpu.N64MEM+Address,(uint8_t*) out, 32);
}

void Abi1::LOADBUFF () { // memcpy causes static... endianess issue :(
	uint32_t v0;
	if (AudioCount == 0)
 		return;
	v0 = (inst2 & 0xfffffc);

	memcpy(BufferSpace+(AudioInBuffer&0xFFFC), (uint8_t*)hCpu.N64MEM+v0, (AudioCount+3)&0xFFFC);

}

void Abi1::SAVEBUFF () { // memcpy causes static... endianess issue :(
	uint32_t v0;
	if (AudioCount == 0)
		return;
	v0 = (inst2 & 0xfffffc);

	memcpy((unsigned char*)hCpu.N64MEM+v0, BufferSpace+(AudioOutBuffer&0xFFFC), (AudioCount+3)&0xFFFC);
}

void Abi1::SEGMENT () { // Should work
	//SEGMENTS[(inst2>>24)&0xf] = (inst2 & 0xffffff);
}

void Abi1::SETBUFF () { // Should work ;-)
	if ((inst1 >> 0x10) & 0x8) { // A_AUX - Auxillary Sound Buffer Settings
		AudioAuxA		= (uint16_t)(inst1);
		AudioAuxC		= (uint16_t)((inst2 >> 0x10));
		AudioAuxE		= (uint16_t)(inst2);
	} else {		// A_MAIN - Main Sound Buffer Settings
		AudioInBuffer   = (uint16_t)(inst1); // 0x00
		AudioOutBuffer	= (uint16_t)((inst2 >> 0x10)); // 0x02
		AudioCount		= (uint16_t)(inst2); // 0x04
	}
}

void Abi1::DMEMMOVE () { // Doesn't sound just right?... will fix when HLE is ready - 03-11-01
	uint32_t v0, v1;
	uint32_t cnt;
	uint32_t count = ((inst2+3) & 0xfffc);
	if ((inst2 & 0xffff)==0)
		return;
	v0 = (inst1 & 0xFFFF);
	v1 = (inst2 >> 0x10);

	for (cnt = 0; cnt < count; cnt++) {
		*(uint8_t *)(BufferSpace+((cnt+v1)^3)) = *(uint8_t *)(BufferSpace+((cnt+v0)^3));
	}
}

void Abi1::LOADADPCM () { // Loads an ADPCM table - Works 100% Now 03-13-01
	uint32_t v0, x;
	v0 = (inst2 & 0xffffff);

	for (x = 0; x < ((inst1 & 0xffff) >> 0x4); x++) {

		uint16_t *table = (uint16_t *)(hCpu.N64MEM+v0+(x<<4));

		adpcmtable[0x1+(x<<3)] = table[0];
		adpcmtable[0x0+(x<<3)] = table[1];

		adpcmtable[0x3+(x<<3)] = table[2];
		adpcmtable[0x2+(x<<3)] = table[3];

		adpcmtable[0x5+(x<<3)] = table[4];
		adpcmtable[0x4+(x<<3)] = table[5];

		adpcmtable[0x7+(x<<3)] = table[6];
		adpcmtable[0x6+(x<<3)] = table[7];

	}
}


void Abi1::INTERLEAVE () { // Works... - 3-11-01
	uint32_t inL, inR;
	uint16_t *inSrcR;
	uint16_t *inSrcL;
	uint16_t Left, Right;
	uint16_t *outbuff = (uint16_t *)(AudioOutBuffer+BufferSpace);
	int x;

	inL = inst2 & 0xFFFF;
	inR = (inst2 >> 16) & 0xFFFF;

	inSrcR = (uint16_t *)(BufferSpace+inR);
	inSrcL = (uint16_t *)(BufferSpace+inL);

	for (x = 0; x < (AudioCount/4); x++) {
		Left=*(inSrcL++);
		Right=*(inSrcR++);

		*(outbuff++)=*(inSrcR++);
		*(outbuff++)=*(inSrcL++);
		*(outbuff++)=(uint16_t)Right;
		*(outbuff++)=(uint16_t)Left;
	}
}


void Abi1::MIXER () { // Fixed a sign issue... 03-14-01
	uint32_t dmemin  = (uint16_t)(inst2 >> 0x10);
	uint32_t dmemout = (uint16_t)(inst2 & 0xFFFF);
	int32_t gain	= (int16_t)(inst1 & 0xFFFF);
	int32_t temp;
	int x;

	if (AudioCount == 0)
		return;

	for (x = 0; x < AudioCount; x+=2) { // I think I can do this a lot easier
		temp = (*(int16_t *)(BufferSpace+dmemin+x) * gain) >> 15;
		temp += *(int16_t *)(BufferSpace+dmemout+x);

		if ((int32_t)temp > 32767)
			temp = 32767;
		if ((int32_t)temp < -32768)
			temp = -32768;

		*(uint16_t *)(BufferSpace+dmemout+x) = (uint16_t)(temp & 0xFFFF);
	}
}


Abi1::Abi1(Cpu & mCpu, uint32_t Special) : ABIBase(mCpu)
{
	Goldeneye = Special;

	memset(segments,0,0x20);
	memset(BufferSpace,0,0x10000);
	memset(hleMixerWorkArea,0,256);
	memset(adpcmtable,0,0x88);

	ABI[0] = HLECAST(&Abi1::SPNOOP);
	ABI[1] = HLECAST(&Abi1::ADPCM);
	ABI[2] = HLECAST(&Abi1::CLEARBUFF);
	ABI[3] = HLECAST(&Abi1::ENVMIXER);
	ABI[4] = HLECAST(&Abi1::LOADBUFF);
	ABI[5] = HLECAST(&Abi1::RESAMPLE);
	ABI[6] = HLECAST(&Abi1::SAVEBUFF);
	ABI[7] = HLECAST(&Abi1::UNKNOWN);
	ABI[8] = HLECAST(&Abi1::SETBUFF);
	ABI[9] = HLECAST(&Abi1::SETVOL);
	ABI[10] = HLECAST(&Abi1::DMEMMOVE);
	ABI[11] = HLECAST(&Abi1::LOADADPCM);
	ABI[12] = HLECAST(&Abi1::MIXER);
	ABI[13] = HLECAST(&Abi1::INTERLEAVE);
	ABI[14] = HLECAST(&Abi1::UNKNOWN);
	ABI[15] = HLECAST(&Abi1::SETLOOP);

	for(int i = 16; i < 32; i++)
		ABI[i] = HLECAST(&Abi1::SPNOOP);
}

Abi1::Abi1(Cpu & mCpu) : ABIBase(mCpu)
{
	Abi1(mCpu, 0);
}

Abi1::~Abi1()
{

}

