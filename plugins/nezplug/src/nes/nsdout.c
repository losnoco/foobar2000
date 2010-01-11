#include "neserr.h"
#include "nestypes.h"
#include "nsdout.h"
#include "nsf6502.h"
#include "nsdplay.h"
#include "m_nsf.h"
#include "songinfo.h"

#define NSDELEMENT 4096
#define MAXDPCMBANK 64

Uint NSD_out_mode = 0;
typedef struct {
	Uint8 nesbank;
	Uint8 *data;
} DPCMBANK;

static struct {
	Uint8 initend;
	Uint8 syncmode;
	Uint32 numsync;
	Uint32 addsync;

	Uint32 nsdoffset;
	Uint32 nsdlimit;
	Uint8 *nsdbase;

	Uint8 banksel[4];
	Uint32 numbanks;
	Uint32 dpcmlimit;
	DPCMBANK banks[MAXDPCMBANK];
} nsdwork;

static Uint8 SearchBank(Uint8 nesbank)
{
	Uint8 nsdbank = 0;
	if (nsdwork.numbanks)
	{
		for (nsdbank = 0; nsdbank < nsdwork.numbanks; nsdbank++)
		{
			if (nsdwork.banks[nsdbank].nesbank == nesbank) return nsdbank;
		}
		if (nsdbank == MAXDPCMBANK) return 0xFF;
	}
	nsdwork.banks[nsdbank].nesbank = nesbank;
	nsdwork.banks[nsdbank].data = XMALLOC(0x1000);
	if (!nsdwork.banks[nsdbank].data) return 0xFF;
	XMEMSET(nsdwork.banks[nsdbank].data, 0, 0x1000);
	nsdwork.dpcmlimit = ++nsdwork.numbanks << 12;
	return nsdwork.numbanks - 1;
}
static void WriteBank(Uint address, Uint value)
{
	Uint8 dpcmbank;
	Uint8 nsdbank;
	dpcmbank = ((address >> 12) & 0xF) - 0xC;
	nsdbank = SearchBank(nsdwork.banksel[dpcmbank]);
	if (nsdbank != 0xFF)
	{
		nsdwork.banks[nsdbank].data[address & 0x0FFF] = value;
	}
}

static void NSDPutc(Uint c)
{
	if (nsdwork.nsdoffset == nsdwork.nsdlimit)
	{
		Uint8 *np = XREALLOC(nsdwork.nsdbase, nsdwork.nsdlimit + NSDELEMENT);
		if (!np) return;
		nsdwork.nsdbase = np;
		nsdwork.nsdlimit += NSDELEMENT;
	}
	nsdwork.nsdbase[nsdwork.nsdoffset++] = c;
}
void NSDStart(Uint syncmode)
{
	NSDTerm(0);
	nsdwork.syncmode = syncmode;
	nsdwork.initend = 0;
	nsdwork.numsync = 0;
	nsdwork.addsync = 0;

	nsdwork.numbanks = 0;
	nsdwork.dpcmlimit = 0;
	nsdwork.banksel[0] = nsdwork.banksel[1] = nsdwork.banksel[2] = nsdwork.banksel[3] = 0xFF;

	nsdwork.nsdoffset = 0;
	nsdwork.nsdlimit = NSDELEMENT;
	nsdwork.nsdbase = XMALLOC(nsdwork.nsdlimit);
	if (!nsdwork.nsdbase) return;
	NSD_out_mode = 1;
}

static void NSDPutVlen(Uint l)
{
	if (l > 0x0FFFFFFF) NSDPutc(((l >> (7 * 4)) & 0x7F) + 0x80);
	if (l > 0x001FFFFF) NSDPutc(((l >> (7 * 3)) & 0x7F) + 0x80);
	if (l > 0x00003FFF) NSDPutc(((l >> (7 * 2)) & 0x7F) + 0x80);
	if (l > 0x0000007F) NSDPutc(((l >> (7 * 1)) & 0x7F) + 0x80);
	NSDPutc(l & 0x7F);
}

static void FrushSync(void)
{
	if (!nsdwork.initend) return;
	if (!nsdwork.syncmode)
	{
		nsdwork.addsync = NES6502GetCycles() - nsdwork.numsync;
	}
	if (nsdwork.addsync == 0)
		return;
	else if (nsdwork.addsync == 1)
	{
		NSDPutc(0xFF);
	}
	else if (nsdwork.addsync == 2)
	{
		NSDPutc(0xFF);
		NSDPutc(0xFF);
	}
	else
	{
		NSDPutc(0xFE);
		NSDPutVlen(nsdwork.addsync - 3);
	}
	nsdwork.numsync += nsdwork.addsync;
	nsdwork.addsync = 0;
}

void NSDWrite(Uint device, Uint address, Uint value)
{
	Uint8 dpcmbank;
	Uint8 nsdbank;
	switch (device)
	{
		case NSD_VSYNC:
			if (nsdwork.syncmode && nsdwork.initend) nsdwork.addsync += 1;
			break;
		case NSD_NSF_MAPPER:
			dpcmbank = address & 0xF;
			if (dpcmbank < 0xC) break;
			dpcmbank -= 0xC;
			nsdwork.banksel[dpcmbank] = value;
			nsdbank = SearchBank(nsdwork.banksel[dpcmbank]);
			FrushSync();
			NSDPutc(0x3C + dpcmbank);
			NSDPutc(nsdbank);
			break;
		case NSD_APU:
			dpcmbank = (address >> 12) & 0xF;
			if (dpcmbank < 0xC)
			{
				FrushSync();
				NSDPutc(address);
				NSDPutc(value);
				break;
			}
			WriteBank(address, value);
			break;
		case NSD_VRC6:
			if ((address & 0x0FFF) < 3)
			{
				FrushSync();
				NSDPutc(0x16 + ((address - 0x9000) >> 12) * 3 + (address & 3));
				NSDPutc(value);
			}
			break;
		case NSD_VRC7:
			FrushSync();
			NSDPutc(0x1F);
			NSDPutc(address);
			NSDPutc(value);
			break;
		case NSD_FDS:
			FrushSync();
			NSDPutc(address);
			NSDPutc(value);
			break;
		case NSD_MMC5:
			FrushSync();
			NSDPutc(0x20 + address - 0x5000);
			NSDPutc(value);
			break;
		case NSD_N106:
			FrushSync();
			NSDPutc(0x36);
			NSDPutc(address);
			NSDPutc(value);
			break;
		case NSD_FME7:
			FrushSync();
			NSDPutc(0x37);
			NSDPutc(address);
			NSDPutc(value);
			break;
		case NSD_INITEND:
			nsdwork.initend = 1;
			nsdwork.numsync = 0;
			break;
	}
}

typedef void (*FNOUTPUT)(void *p, Uint l);
static Uint CountZero(Uint a)
{
	Uint c;
	for (c = 0; a < nsdwork.dpcmlimit; a++, c++)
	{
		if (nsdwork.banks[a >> 12].data[a & 0x0FFF]) return c;
	}
	return c;
}
static Uint CountNonZero(Uint a)
{
	Uint c;
	for (c = 0; a < nsdwork.dpcmlimit; a++, c++)
	{
		if (!nsdwork.banks[a >> 12].data[a & 0x0FFF]) return c;
	}
	return c;
}
static void OutputVlen(FNOUTPUT fnOutput, Uint *outlen, Uint l)
{
	Uint8 buf[8];
	Uint8 *bufp = buf;
	if (l > 0x0FFFFFFF) *bufp++ = ((l >> (7 * 4)) & 0x7F) + 0x80;
	if (l > 0x001FFFFF) *bufp++ = ((l >> (7 * 3)) & 0x7F) + 0x80;
	if (l > 0x00003FFF) *bufp++ = ((l >> (7 * 2)) & 0x7F) + 0x80;
	if (l > 0x0000007F) *bufp++ = ((l >> (7 * 1)) & 0x7F) + 0x80;
	*bufp++ = l & 0x7F;
	if (fnOutput) fnOutput(buf, bufp - buf);
	*outlen += bufp - buf;
}

static void OutputZero(FNOUTPUT fnOutput, Uint *outlen, Uint l)
{
	OutputVlen(fnOutput, outlen, (l << 1));
}
static void OutputNonZero(FNOUTPUT fnOutput, Uint *outlen, Uint a, Uint l)
{
	Uint r;
	OutputVlen(fnOutput, outlen, (l << 1) + 1);
	while (l)
	{
		r = 0x1000 - (a & 0xFFF);
		if (r > l) r = l;
		if (fnOutput) fnOutput(&nsdwork.banks[a >> 12].data[a & 0x0FFF], r);
		a += r;
		l -= r;
		*outlen += r;
	}
}

static Uint OutputDpcmInfo(FNOUTPUT fnOutput)
{
	Uint a = 0;
	Uint outlen = 0;
	OutputVlen(fnOutput, &outlen, nsdwork.numbanks);
	while (a < nsdwork.dpcmlimit)
	{
		Uint b = a;
		Uint cnz = 0;
		do {
			Uint cz = CountZero(a);
			if (cz > 2)
			{
				if (a != b)
				{
					OutputNonZero(fnOutput, &outlen, b, a - b);
				}
				OutputZero(fnOutput, &outlen, cz);
				a += cz;
				b = a;
				cnz = 0;
				break;
			}
			a += cz;
			cnz = CountNonZero(a);
			a += cnz;
		} while (cnz);
		if (a != b)
		{
			OutputNonZero(fnOutput, &outlen, b, a - b);
			b = a;
		}
	}
	return outlen;
}

static void SetDwordLE(Uint8 *p, Uint32 v)
{
	p[0] = (v >> (8 * 0)) & 0xFF;
	p[1] = (v >> (8 * 1)) & 0xFF;
	p[2] = (v >> (8 * 2)) & 0xFF;
	p[3] = (v >> (8 * 3)) & 0xFF;
}

void NSDTerm(FNOUTPUT fnOutput)
{
	Uint8 head[0x80];
	Uint8 nsdbank;
	Uint dpcminfolen = 0;
	if (!NSD_out_mode) return;
	NSDPutc(0xFD);
	NSD_out_mode = 0;
	if (nsdwork.numbanks) dpcminfolen = OutputDpcmInfo(0);
	XMEMSET(head, 0, sizeof(head));
	XMEMCPY(head, "NESL\x1A", 5);
	head[0x07] = nsdwork.syncmode ? 1 : 0;
	head[0x0C] = SONGINFO_GetExtendDevice();
	if (dpcminfolen)
	{
		SetDwordLE(head + 0x20, dpcminfolen);
		SetDwordLE(head + 0x28, sizeof(head));
	}
	SetDwordLE(head + 0x30, nsdwork.nsdoffset);
	SetDwordLE(head + 0x38, sizeof(head) + dpcminfolen);
	if (fnOutput)
	{
		fnOutput(head, sizeof(head));
		OutputDpcmInfo(fnOutput);
		fnOutput(nsdwork.nsdbase, nsdwork.nsdoffset);
	}
	for (nsdbank = 0; nsdbank < nsdwork.numbanks; nsdbank++)
	{
		XFREE(nsdwork.banks[nsdbank].data);
	}
	XFREE(nsdwork.nsdbase);
}
