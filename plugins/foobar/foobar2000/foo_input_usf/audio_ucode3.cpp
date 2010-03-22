#include <stdio.h>
#include "usf.h"
#include "audio_hle.h"




void Abi3::SPNOOP ()
{
}



void Abi3::SETVOL3 () {
	uint8_t Flags = (uint8_t)(inst1 >> 0x10);
	if (Flags & 0x4) { // 288
		if (Flags & 0x2) { // 290
			Vol_Left  = *(int16_t*)&inst1; // 0x50
			Env_Dry		= (int16_t)(*(int32_t*)&inst2 >> 0x10); // 0x4E
			Env_Wet		= *(int16_t*)&inst2; // 0x4C
		} else {
			VolTrg_Right  = *(int16_t*)&inst1; // 0x46
			//VolRamp_Right = (uint16_t)(inst2 >> 0x10) | (int32_t)(int16_t)(inst2 << 0x10);
			VolRamp_Right = *(int32_t*)&inst2; // 0x48/0x4A
		}
	} else {
		VolTrg_Left  = *(int16_t*)&inst1; // 0x40
		VolRamp_Left = *(int32_t*)&inst2; // 0x42/0x44
	}
}

void Abi3::ENVMIXER3 () {
	uint8_t flags;
	uint32_t addy;

 	short *inp=(short *)(BufferSpace+0x4F0);
	short *out=(short *)(BufferSpace+0x9D0);
	short *aux1=(short *)(BufferSpace+0xB40);
	short *aux2=(short *)(BufferSpace+0xCB0);
	short *aux3=(short *)(BufferSpace+0xE20);
	int32_t MainR;
	int32_t MainL;
	int32_t AuxR;
	int32_t AuxL;

	int i1,o1,a1,a2,a3,x,y;
	//WORD AuxIncRate=1;
	short zero[8];
	memset(zero,0,16);

	flags = (uint8_t)((inst1 >> 16) & 0xff);
	addy = (inst2 & 0xFFFFFF);

	int32_t LAdder, LAcc, LVol;
	int32_t RAdder, RAcc, RVol;
	int16_t RSig, LSig; // Most significant part of the Ramp Value
	int16_t Wet, Dry;
	int16_t LTrg, RTrg;


	Vol_Right = (*(int16_t *)&inst1);

	if (flags & A_INIT) {
		LAdder = VolRamp_Left / 8;
		LAcc  = 0;
		LVol  = Vol_Left;
		LSig = (int16_t)(VolRamp_Left >> 16);

		RAdder = VolRamp_Right / 8;
		RAcc  = 0;
		RVol  = Vol_Right;
		RSig = (int16_t)(VolRamp_Right >> 16);

		Wet = (int16_t)Env_Wet; Dry = (int16_t)Env_Dry; // Save Wet/Dry values
		LTrg = VolTrg_Left; RTrg = VolTrg_Right; // Save Current Left/Right Targets
	} else {
		memcpy((uint8_t *)hleMixerWorkArea, hCpu.N64MEM+addy, 80);
		Wet	= *(int16_t *)(hleMixerWorkArea +  0); // 0-1
		Dry	= *(int16_t *)(hleMixerWorkArea +  2); // 2-3
		LTrg   = *(int16_t *)(hleMixerWorkArea +  4); // 4-5
		RTrg   = *(int16_t *)(hleMixerWorkArea +  6); // 6-7
		LAdder = *(int32_t *)(hleMixerWorkArea +  8); // 8-9 (hleMixerWorkArea is a 16bit pointer)
		RAdder = *(int32_t *)(hleMixerWorkArea + 10); // 10-11
		LAcc   = *(int32_t *)(hleMixerWorkArea + 12); // 12-13
		RAcc   = *(int32_t *)(hleMixerWorkArea + 14); // 14-15
		LVol   = *(int32_t *)(hleMixerWorkArea + 16); // 16-17
		RVol   = *(int32_t *)(hleMixerWorkArea + 18); // 18-19
		LSig   = *(int16_t *)(hleMixerWorkArea + 20); // 20-21
		RSig   = *(int16_t *)(hleMixerWorkArea + 22); // 22-23
		//u32 test  = *(int32_t *)(hleMixerWorkArea + 24); // 22-23
		//if (test != 0x13371337)
		//	__asm int 3;
	}


	//if(!(flags&A_AUX)) {
	//	AuxIncRate=0;
	//	aux2=aux3=zero;
	//}

	for (y = 0; y < (0x170/2); y++) {

		// Left
		LAcc += LAdder;
		LVol += (LAcc >> 16);
		LAcc &= 0xFFFF;

		// Right
		RAcc += RAdder;
		RVol += (RAcc >> 16);
		RAcc &= 0xFFFF;
// ****************************************************************
		// Clamp Left
		if (LSig >= 0) { // VLT
			if (LVol > LTrg) {
				LVol = LTrg;
			}
		} else { // VGE
			if (LVol < LTrg) {
				LVol = LTrg;
			}
		}

		// Clamp Right
		if (RSig >= 0) { // VLT
			if (RVol > RTrg) {
				RVol = RTrg;
			}
		} else { // VGE
			if (RVol < RTrg) {
				RVol = RTrg;
			}
		}
// ****************************************************************
		MainL = ((Dry * LVol) + 0x4000) >> 15;
		MainR = ((Dry * RVol) + 0x4000) >> 15;

		o1 = out [y^1];
		a1 = aux1[y^1];
		i1 = inp [y^1];

		o1+=((i1*MainL)+0x4000)>>15;
		a1+=((i1*MainR)+0x4000)>>15;

// ****************************************************************

		if(o1>32767) o1=32767;
		else if(o1<-32768) o1=-32768;

		if(a1>32767) a1=32767;
		else if(a1<-32768) a1=-32768;

// ****************************************************************

		out[y^1]=o1;
		aux1[y^1]=a1;

// ****************************************************************
		//if (!(flags&A_AUX)) {
			a2 = aux2[y^1];
			a3 = aux3[y^1];

			AuxL  = ((Wet * LVol) + 0x4000) >> 15;
			AuxR  = ((Wet * RVol) + 0x4000) >> 15;

			a2+=((i1*AuxL)+0x4000)>>15;
			a3+=((i1*AuxR)+0x4000)>>15;

			if(a2>32767) a2=32767;
			else if(a2<-32768) a2=-32768;

			if(a3>32767) a3=32767;
			else if(a3<-32768) a3=-32768;

			aux2[y^1]=a2;
			aux3[y^1]=a3;
		}
	//}

	*(int16_t *)(hleMixerWorkArea +  0) = Wet; // 0-1
	*(int16_t *)(hleMixerWorkArea +  2) = Dry; // 2-3
	*(int16_t *)(hleMixerWorkArea +  4) = LTrg; // 4-5
	*(int16_t *)(hleMixerWorkArea +  6) = RTrg; // 6-7
	*(int32_t *)(hleMixerWorkArea +  8) = LAdder; // 8-9 (hleMixerWorkArea is a 16bit pointer)
	*(int32_t *)(hleMixerWorkArea + 10) = RAdder; // 10-11
	*(int32_t *)(hleMixerWorkArea + 12) = LAcc; // 12-13
	*(int32_t *)(hleMixerWorkArea + 14) = RAcc; // 14-15
	*(int32_t *)(hleMixerWorkArea + 16) = LVol; // 16-17
	*(int32_t *)(hleMixerWorkArea + 18) = RVol; // 18-19
	*(int16_t *)(hleMixerWorkArea + 20) = LSig; // 20-21
	*(int16_t *)(hleMixerWorkArea + 22) = RSig; // 22-23
	//*(uint32_t *)(hleMixerWorkArea + 24) = 0x13371337; // 22-23
	memcpy(hCpu.N64MEM+addy, (uint8_t *)hleMixerWorkArea,80);
}

void Abi3::CLEARBUFF3 () {
	uint16_t addr = (uint16_t)(inst1 & 0xffff);
	uint16_t count = (uint16_t)(inst2 & 0xffff);
	memset(BufferSpace+addr+0x4f0, 0, count);
}

void Abi3::MIXER3 () { // Needs accuracy verification...
	uint16_t dmemin  = (uint16_t)(inst2 >> 0x10)  + 0x4f0;
	uint16_t dmemout = (uint16_t)(inst2 & 0xFFFF) + 0x4f0;
	int32_t gain	= (int16_t)(inst1 & 0xFFFF)*2;
	int32_t temp;
	int x;

	for (x = 0; x < 0x170; x+=2) { // I think I can do this a lot easier
		temp = (*(int16_t *)(BufferSpace+dmemin+x) * gain) >> 16;
		temp += *(int16_t *)(BufferSpace+dmemout+x);

		if ((int32_t)temp > 32767)
			temp = 32767;
		if ((int32_t)temp < -32768)
			temp = -32768;

		*(uint16_t *)(BufferSpace+dmemout+x) = (uint16_t)(temp & 0xFFFF);
	}
}

void Abi3::LOADBUFF3 () {
	uint32_t v0;
	uint32_t cnt = (((inst1 >> 0xC)+3)&0xFFC);
	v0 = (inst2 & 0xfffffc);
	uint32_t src = (inst1&0xffc)+0x4f0;
	memcpy (BufferSpace+src, (uint8_t *)hCpu.N64MEM+v0, cnt);
}

void Abi3::SAVEBUFF3 () {
	uint32_t v0;
	uint32_t cnt = (((inst1 >> 0xC)+3)&0xFFC);
	v0 = (inst2 & 0xfffffc);
	uint32_t src = (inst1&0xffc)+0x4f0;
	memcpy (hCpu.N64MEM+v0, (uint8_t *)BufferSpace+src, cnt);
}

void Abi3::LOADADPCM3 () { // Loads an ADPCM table - Works 100% Now 03-13-01
	uint32_t v0;
	v0 = (inst2 & 0xffffff);

	uint16_t *table = (uint16_t *)(hCpu.N64MEM+v0);
	for (uint32_t x = 0; x < ((inst1&0xffff)>>0x4); x++) {
		adpcmtable[0x1+(x<<3)] = table[0];
		adpcmtable[0x0+(x<<3)] = table[1];

		adpcmtable[0x3+(x<<3)] = table[2];
		adpcmtable[0x2+(x<<3)] = table[3];

		adpcmtable[0x5+(x<<3)] = table[4];
		adpcmtable[0x4+(x<<3)] = table[5];

		adpcmtable[0x7+(x<<3)] = table[6];
		adpcmtable[0x6+(x<<3)] = table[7];
		table += 8;
	}
}

void Abi3::DMEMMOVE3 () { // Needs accuracy verification...
	uint32_t v0, v1;
	uint32_t cnt;
	v0 = (inst1 & 0xFFFF) + 0x4f0;
	v1 = (inst2 >> 0x10) + 0x4f0;
	uint32_t count = ((inst2+3) & 0xfffc);

	for (cnt = 0; cnt < count; cnt++) {
		*(uint8_t *)(BufferSpace+((cnt+v1)^3)) = *(uint8_t *)(BufferSpace+((cnt+v0)^3));
	}
}

void Abi3::SETLOOP3 () {
	loopval = (inst2 & 0xffffff);
}

void Abi3::ADPCM3 () { // Verified to be 100% Accurate...
	short count = 0;
	unsigned char icode, code, Flags;
	int vscale, a[8];
	unsigned short index, j, inPtr, x;
	short *book1,*book2,*out;
	unsigned int Address;

	Flags=(uint8_t)(inst2>>0x1c)&0xff;
	Address=(inst1 & 0xffffff);
	inPtr=(inst2>>12)&0xf;

	out=(short *)(BufferSpace+(inst2&0xfff)+0x4f0);
	count=(short)((inst2 >> 16)&0xfff);


	memset(out,0,32);

	if(!(Flags&0x1))
	{
		if(Flags&0x2)
		{
			memcpy(out,&hCpu.N64MEM[loopval],32);
		}
		else
		{
			memcpy(out,&hCpu.N64MEM[Address],32);
		}
	}

	int l1=out[15];
	int l2=out[14];
	int inp1[8];
	int inp2[8];
	out+=16;
	while(count>0)
	{
													// the first interation through, these values are
													// either 0 in the case of A_INIT, from a special
													// area of memory in the case of A_LOOP or just
													// the values we calculated the last time

		code=BufferSpace[(0x4f0+inPtr)^3];
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
			icode=BufferSpace[(0x4f0+inPtr)^3];
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
			icode=BufferSpace[(0x4f0+inPtr)^3];
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
	memcpy(&hCpu.N64MEM[Address],out,32);
}

void Abi3::RESAMPLE3 () {
	int32_t temp;
	int32_t accum;
	int16_t *lut;
	short *dst;
	int16_t *src;
	unsigned char Flags;
	uint32_t dstPtr, srcPtr, x, y, addy, Pitch, location, Accum, i;
	Flags=(uint8_t)((inst2>>0x1e));
	Pitch=((inst2>>0xe)&0xffff)<<1;
	addy = (inst1 & 0xffffff);
	Accum=0;
	dst=(short *)(BufferSpace);
	src=(int16_t *)(BufferSpace);
	srcPtr=((((inst2>>2)&0xfff)+0x4f0)/2);

	srcPtr -= 4;

	if (inst2 & 0x3) {
		dstPtr = 0x660/2;
	} else {
		dstPtr = 0x4f0/2;
	}

	if ((Flags & 0x1) == 0) {
		for (int x=0; x < 4; x++) //memcpy (src+srcPtr, rsp.hCpu.N64MEM+addy, 0x8);
			src[(srcPtr+x)^1] = ((uint16_t *)hCpu.N64MEM)[((addy/2)+x)^1];
		Accum = *(uint16_t *)(hCpu.N64MEM+addy+10);
	} else {
		for (int x=0; x < 4; x++)
			src[(srcPtr+x)^1] = 0;//*(uint16_t *)(rsp.hCpu.N64MEM+((addy+x)^2));
	}

	//if ((Flags & 0x2))
	//	__asm int 3;

	for(int i=0;i < 0x170/2;i++)	{
		location = (((Accum * 0x40) >> 0x10) * 8);
		//location = (Accum >> 0xa) << 0x3;
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
	for (int x=0; x < 4; x++)
		((uint16_t *)hCpu.N64MEM)[((addy/2)+x)^1] = src[(srcPtr+x)^1];
	*(uint16_t *)(hCpu.N64MEM+addy+10) = Accum;
}

void Abi3::INTERLEAVE3 () { // Needs accuracy verification...
	//u32 inL, inR;
	int x = 0;
	uint16_t *inSrcR;
	uint16_t *inSrcL;
	uint16_t Left, Right;

	uint16_t *outbuff = (uint16_t *)(BufferSpace + 0x4f0);//(uint16_t *)(AudioOutBuffer+dmem);

	inSrcR = (uint16_t *)(BufferSpace+0xb40);
	inSrcL = (uint16_t *)(BufferSpace+0x9d0);

	for (x = 0; x < (0x170/4); x++) {
		Left=*(inSrcL++);
		Right=*(inSrcR++);

		*(outbuff++)=*(inSrcR++);
		*(outbuff++)=*(inSrcL++);
		*(outbuff++)=(uint16_t)Right;
		*(outbuff++)=(uint16_t)Left;
	}
}

void Abi3::WHATISTHIS () {
}

uint32_t setaddr;
void Abi3::MP3ADDY () {
	setaddr = (inst2 & 0xffffff);
}



extern uint32_t base, dmembase;

extern char *pDMEM;

void MP3 ();

void Abi3::DISABLE () {
}


Abi3::Abi3(Cpu & mCpu) : ABIBase(mCpu)
{


	memset(BufferSpace,0,0x10000);

	memset(adpcmtable,0,0x88*2);
	AudioInBuffer = 0;
	AudioOutBuffer = 0;
	AudioCount = 0;
	loopval = 0;

	ABI[0] = HLECAST(&Abi3::DISABLE);
	ABI[1] = HLECAST(&Abi3::ADPCM3);
	ABI[2] = HLECAST(&Abi3::CLEARBUFF3);
	ABI[3] = HLECAST(&Abi3::ENVMIXER3);
	ABI[4] = HLECAST(&Abi3::LOADBUFF3);
	ABI[5] = HLECAST(&Abi3::RESAMPLE3);
	ABI[6] = HLECAST(&Abi3::SAVEBUFF3);
	ABI[7] = HLECAST(&Abi3::MP3);
	ABI[8] = HLECAST(&Abi3::MP3ADDY);
	ABI[9] = HLECAST(&Abi3::SETVOL3);
	ABI[10] = HLECAST(&Abi3::DMEMMOVE3);
	ABI[11] = HLECAST(&Abi3::LOADADPCM3);
	ABI[12] = HLECAST(&Abi3::MIXER3);
	ABI[13] = HLECAST(&Abi3::INTERLEAVE3);
	ABI[14] = HLECAST(&Abi3::WHATISTHIS);
	ABI[15] = HLECAST(&Abi3::SETLOOP3);

	for(int i = 16; i < 32; i++)
		ABI[i] = HLECAST(&Abi3::SPNOOP);

}

Abi3::~Abi3()
{

}
