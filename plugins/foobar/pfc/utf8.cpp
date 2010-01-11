#include "pfc.h"

//utf8 stuff

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef UINT
typedef unsigned int UINT;
#endif

static const BYTE mask_tab[6]={0x80,0xE0,0xF0,0xF8,0xFC,0xFE};

static const BYTE val_tab[6]={0,0xC0,0xE0,0xF0,0xF8,0xFC};

unsigned utf8_char_len_from_header(char p_c)
{
	t_uint8 c = (t_uint8)p_c;

	unsigned cnt = 0;
	for(;;)
	{
		if ((p_c & mask_tab[cnt])==val_tab[cnt]) break;
		if (++cnt>=6) return 0;
	}

	return cnt + 1;

}

unsigned utf8_decode_char(const char *p_utf8,unsigned * wide,unsigned max)
{
	const t_uint8 * utf8 = (const t_uint8*)p_utf8;
	
	if (wide) *wide = 0;

	if (max==0)
		return 0;
	else if (max>6) max = 6;

	if (utf8[0]<0x80)
	{
		if (wide) *wide = utf8[0];
		return utf8[0]>0 ? 1 : 0;
	}

	unsigned res=0;
	unsigned n;
	unsigned cnt=0;
	for(;;)
	{
		if ((*utf8&mask_tab[cnt])==val_tab[cnt]) break;
		if (++cnt>=max) return 0;
	}
	cnt++;

	for(n=0;n<cnt;n++)
		if (utf8[n]==0) return 0;


	if (cnt==2 && !(*utf8&0x1E)) return 0;

	if (cnt==1)
		res=*utf8;
	else
		res=(0xFF>>(cnt+1))&*utf8;

	for (n=1;n<cnt;n++)
	{
		if ((utf8[n]&0xC0) != 0x80)
			return 0;
		if (!res && n==2 && !((utf8[n]&0x7F) >> (7 - cnt)))
			return 0;

		res=(res<<6)|(utf8[n]&0x3F);
	}

	if (wide)
		*wide=res;

	return cnt;
}


unsigned utf8_encode_char(unsigned wide,char * target)
{
	unsigned int count;

	if (wide < 0x80)
		count = 1;
	else if (wide < 0x800)
		count = 2;
	else if (wide < 0x10000)
		count = 3;
	else if (wide < 0x200000)
		count = 4;
	else if (wide < 0x4000000)
		count = 5;
	else if (wide <= 0x7FFFFFFF)
		count = 6;
	else
		return 0;
	//if (count>max) return 0;

	if (target == 0)
		return count;

	switch (count)
	{
    case 6:
		target[5] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x4000000;
    case 5:
		target[4] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x200000;
    case 4:
		target[3] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x10000;
    case 3:
		target[2] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x800;
    case 2:
		target[1] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0xC0;
	case 1:
		target[0] = wide;
	}

	return count;
}

unsigned utf16_encode_char(unsigned cur_wchar,WCHAR * out)
{
	if (cur_wchar>0 && cur_wchar<(1<<20))
	{
		if (cur_wchar>=0x10000)
		{
			unsigned c = cur_wchar - 0x10000;
			//MSDN:
			//The first (high) surrogate is a 16-bit code value in the range U+D800 to U+DBFF. The second (low) surrogate is a 16-bit code value in the range U+DC00 to U+DFFF. Using surrogates, Unicode can support over one million characters. For more details about surrogates, refer to The Unicode Standard, version 2.0.
			out[0] = (WCHAR)(0xD800 | (0x3FF & (c>>10)) );
			out[1] = (WCHAR)(0xDC00 | (0x3FF & c) ) ;
			return 2;
		}
		else
		{
			*out = (WCHAR)cur_wchar;
			return 1;
		}
	}
	return 0;
}

unsigned utf16_decode_char(const wchar_t * p_source,unsigned * p_out,unsigned p_source_length) {
	if (p_source_length == 0) return 0;
	else if (p_source_length == 1) {
		*p_out = p_source[0];
		return 1;
	} else {
		unsigned retval = 0;
		unsigned decoded = p_source[0];
		if (decoded != 0)
		{
			retval = 1;
			if ((decoded & 0xFC00) == 0xD800)
			{
				unsigned low = p_source[1];
				if ((low & 0xFC00) == 0xDC00)
				{
					decoded = 0x10000 + ( ((decoded & 0x3FF) << 10) | (low & 0x3FF) );
					retval = 2;
				}
			}
		}
		*p_out = decoded;
		return retval;
	}
}


UINT utf8_get_char(const char * src)
{
	UINT rv = 0;
	utf8_decode_char(src,&rv);
	return rv;
}


unsigned utf8_char_len(const char * s,unsigned max)
{
	return utf8_decode_char(s,0,max);
}

unsigned skip_utf8_chars(const char * ptr,unsigned count)
{
	int num = 0;
	for(;count && ptr[num];count--)
	{
		int d = utf8_char_len(ptr+num);
		if (d<=0) break;
		num+=d;		
	}
	return num;
}

bool is_valid_utf8(const char * param)
{
	__try {
		while(*param)
		{
			unsigned d;
			d = utf8_decode_char(param,0);
			if (d==0) return false;
			param += d;
		}
		return true;
	}
	__except(1)
	{
		return false;
	}
}

bool is_lower_ascii(const char * param)
{
	while(*param)
	{
		if (*param<0) return false;
		param++;
	}
	return true;
}

static bool check_end_of_string(const char * ptr)
{
	__try {
		return !*ptr;
	}
	__except(1) {return true;}
}

unsigned strcpy_utf8_truncate(const char * src,char * out,unsigned maxbytes)
{
	unsigned rv = 0 , ptr = 0;
	if (maxbytes>0)
	{	
		maxbytes--;//for null
		while(!check_end_of_string(src) && maxbytes>0)
		{
			__try {
				unsigned delta = utf8_char_len(src);
				if (delta>maxbytes || delta==0) break;
				do 
				{
					out[ptr++] = *(src++);
				} while(--delta);
			} __except(1) { break; }
			rv = ptr;
		}
		out[rv]=0;
	}
	return rv;
}

unsigned string8::replace_char(unsigned c1,unsigned c2,unsigned start)
{
	if (c1 < 128 && c2 < 128) return replace_byte((char)c1,(char)c2,start);

	string8 temp(get_ptr()+start);
	truncate(start);
	const char * ptr = temp;
	unsigned rv = 0;
	while(*ptr)
	{
		unsigned test;
		unsigned delta = utf8_decode_char(ptr,&test);
		if (delta==0 || test==0) break;
		if (test == c1) {test = c2;rv++;}
		add_char(test);
		ptr += delta;
	}
	return rv;
}

unsigned strlen_utf8(const char * p,unsigned num)
{
	unsigned w,d;
	unsigned ret = 0;
	for(;num;)
	{
		d = utf8_decode_char(p,&w);
		if (w==0 || d<=0) break;
		ret++;
		p+=d;
		num-=d;
	}
	return ret;
}

unsigned utf8_chars_to_bytes(const char * string,unsigned count)
{
	unsigned bytes = 0;
	while(count)
	{
		unsigned delta = utf8_decode_char(string+bytes,0);
		if (delta==0) break;
		bytes += delta;
		count--;
	}
	return bytes;
}