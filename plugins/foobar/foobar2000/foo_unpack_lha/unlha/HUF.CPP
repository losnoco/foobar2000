/***********************************************************
	huf.c -- new static Huffman
***********************************************************/

#define NP (MAX_DICBIT + 1)
#define NT (USHRT_BIT + 3)
//#define PBIT 4  /* smallest integer such that (1 << PBIT) > NP */
#define TBIT 5  /* smallest integer such that (1 << TBIT) > NT */
#define NPT 0x80

typedef struct _HUFDATA
{
	unsigned char c_len[CLhaArchive::NC], pt_len[NPT];
	unsigned short c_table[4096], pt_table[256];
} HUFDATA;

//HUFDATA *gpHufData = NULL;

#define gpHufData (*((HUFDATA **)&m_pHufData))

VOID CLhaArchive::InitHufTables()
{
	gpHufData = (HUFDATA *)m_pDecoderData;
}


/***** decoding *****/

void CLhaArchive::read_pt_len(short nn, short nbit, short i_special)
{
	int i, c, n;

	n = getbits(nbit);
	if (n == 0) {
		c = getbits(nbit);
		for (i = 0; i < nn; i++) gpHufData->pt_len[i] = 0;
		for (i = 0; i < 256; i++) gpHufData->pt_table[i] = c;
	} else {
		i = 0;
		while (i < min(n, NPT)) {
			c = bitbuf >> (16 - 3);
			if ( c != 7 )
				fillbuf( 3 );
			else
			{
				unsigned short mask = 1 << (16 - 4);
				while (mask & bitbuf) {  mask >>= 1;  c++;  }
				fillbuf( c - 3 );
			}
			gpHufData->pt_len[i++] = (unsigned char)c;
			if (i == i_special) {
				c = getbits(2);
				while (--c >= 0 && i < NPT) gpHufData->pt_len[i++] = 0;
			}
		}
		while (i < nn) gpHufData->pt_len[i++] = 0;
		make_table(nn, gpHufData->pt_len, 8, gpHufData->pt_table);
	}
}

void CLhaArchive::read_c_len()
{
	int i, c, n;

	n = getbits(CBIT);
	if (n == 0) {
		c = getbits(CBIT);
		for (i = 0; i < NC; i++) gpHufData->c_len[i] = 0;
		for (i = 0; i < 4096; i++) gpHufData->c_table[i] = c;
	} else {
		i = 0;
		while (i < min(n,NC)) {
			c = gpHufData->pt_table[bitbuf >> (16 - 8)];
			if (c >= NT) {
				unsigned short mask = 1 << (16 - 9);
				do {
					c = (bitbuf & mask) ? right[c] : left[c];
					mask >>= 1;
				} while (c >= NT && (mask || c != left[c])); /* CVE-2006-4338 */
			}
			fillbuf(gpHufData->pt_len[c]);
			if (c <= 2) {
				if      (c == 0) c = 1;
				else if (c == 1) c = getbits(4) + 3;
				else             c = getbits(CBIT) + 20;
				while (--c >= 0) gpHufData->c_len[i++] = 0;
			} else gpHufData->c_len[i++] = c - 2;
		}
		while (i < NC) gpHufData->c_len[i++] = 0;
		make_table(NC, gpHufData->c_len, 12, gpHufData->c_table);
	}
}

unsigned short CLhaArchive::decode_c_st1()
{
	unsigned short j, mask;

#ifdef LHADEBUG
	if (!gpHufData)
	{
		Log("gpHufData is NULL!!!!");
		return 0;
	}
#endif
	if (blocksize == 0)
	{
		blocksize = getbits(16);
		read_pt_len(NT, TBIT, 3);
		read_c_len();
		read_pt_len(np, pbit, -1);
	}
	blocksize--;
	j = gpHufData->c_table[bitbuf >> 4];
	if (j < NC) fillbuf(gpHufData->c_len[j]);
	else {
		fillbuf(12);  mask = 1 << (16 - 1);
		do {
			j = (bitbuf & mask) ? right[j] : left[j];
			mask >>= 1;
		} while (j >= NC && (mask || j != left[j])); /* CVE-2006-4338 */
		fillbuf(gpHufData->c_len[j] - 12);
	}
	return j;
}

unsigned short CLhaArchive::decode_p_st1()
{
	unsigned short j, mask;

	j = gpHufData->pt_table[bitbuf >> (16 - 8)];
	if (j < np) fillbuf(gpHufData->pt_len[j]);
	else {
		fillbuf(8);  mask = 1 << (16 - 1);
		do {
			j = (bitbuf & mask) ? right[j] : left[j];
			mask >>= 1;
		} while (j >= np && (mask || j != left[j])); /* CVE-2006-4338 */
		fillbuf(gpHufData->pt_len[j] - 8);
	}
	if (j != 0) j = (1 << (j - 1)) + getbits(j - 1);
	return j;
}

void CLhaArchive::decode_start_st1(int dicbit)
{
	switch (dicbit)
	{
    case LZHUFF4_DICBIT:
    case LZHUFF5_DICBIT: pbit = 4; np = LZHUFF5_DICBIT + 1; break;
    case LZHUFF6_DICBIT: pbit = 5; np = LZHUFF6_DICBIT + 1; break;
    case LZHUFF7_DICBIT: pbit = 5; np = LZHUFF7_DICBIT + 1; break;
	}
	init_getbits();
	blocksize = 0;
}
