#include "shared.h"

#include <locale.h>

extern "C" {

#if 0
inline static unsigned q_tolower(unsigned c)
{
	if (c>='A' && c<='Z') c += 'a' - 'A';
	return c;
}
#else
static const t_uint8 ascii_tolower_table[128] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x5B,0x5C,0x5D,0x5E,0x5F,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F};
#define q_tolower(c) ascii_tolower_table[c]
#endif

#ifdef WIN32
static UINT char_convert_win9x(UINT param,bool b_upper)
{
	assert(param && param<0x10000);
	char temp[16];
	WCHAR temp_w[16];
	temp_w[0] = (WCHAR)param;
	temp_w[1] = 0;
	if (WideCharToMultiByte(CP_ACP,0,temp_w,-1,temp,16,0,0)==0) return param;
	temp[15] = 0;
	if (b_upper) CharUpperA(temp); else CharLowerA(temp);
	if (MultiByteToWideChar(CP_ACP,0,temp,-1,temp_w,16)==0) return param;
	if (temp_w[0]==0 || temp_w[1]!=0) return param;
	return temp_w[0];
}

#endif

SHARED_EXPORT UINT uCharLower(UINT param)
{
	if (param<128)
	{
		if (param>='A' && param<='Z') param += 'a' - 'A';
		return param;
	}
#ifdef WIN32
	else if (param<0x10000)
	{
		unsigned ret;
#ifdef UNICODE
		ret = (unsigned)CharLowerW((WCHAR*)param);
#else
		ret = char_convert_win9x(param,false);
#endif
		return ret;
	}
	else return param;
#else
	else
	{
		setlocale(LC_CTYPE,"");
		return towlower(param);
	}
#endif
}

SHARED_EXPORT UINT uCharUpper(UINT param)
{
	if (param<128)
	{
		if (param>='a' && param<='z') param += 'A' - 'a';
		return param;
	}
#ifdef WIN32
	else if (param<0x10000)
	{
		unsigned ret;
#ifdef UNICODE
		ret = (unsigned)CharUpperW((WCHAR*)param);
#else
		ret = char_convert_win9x(param,true);
#endif
		return ret;
	}
	else return param;
#else
	else
	{
		setlocale(LC_CTYPE,"");
		return towupper(param);
	}
#endif
}

static inline int compare_wchar(unsigned c1,unsigned c2)
{
	if (c1==c2) return 0;
	c1 = char_lower(c1);
	c2 = char_lower(c2);
	if (c1<c2) return -1;
	else if (c1>c2) return 1;
	else return 0;
}


SHARED_EXPORT int stricmp_utf8(const char * p1,const char * p2)
{
	for(;;)
	{
		if (*p1>=0 && *p2>=0)//signed char
		{
			unsigned c1 = q_tolower(*p1), c2 = q_tolower(*p2);
			if (c1<c2) return -1;
			else if (c1>c2) return 1;
			else if (c1 == 0) return 0;
			else
			{
				p1++;
				p2++;
			}
		}
		else
		{
			unsigned w1,w2,d1,d2;
			d1 = utf8_decode_char(p1,&w1);
			d2 = utf8_decode_char(p2,&w2);
			if (d1==0) return -1;
			if (d2==0) return 1;
			int rv = compare_wchar(w1,w2);
			if (rv) return rv;
			p1 += d1;
			p2 += d2;
		}
	}
}

SHARED_EXPORT int stricmp_utf8_stringtoblock(const char * p1,const char * p2,unsigned p2_bytes)
{
	return stricmp_utf8_ex(p1,-1,p2,p2_bytes);
}

SHARED_EXPORT int stricmp_utf8_partial(const char * p1,const char * p2,unsigned num)
{
	for(;num;)
	{
		unsigned int w1,w2,d1,d2;
		d1 = utf8_decode_char(p1,&w1);
		d2 = utf8_decode_char(p2,&w2);
		if (w2==0 || d2==0) return 0;
		int rv = compare_wchar(w1,w2);
		if (rv) return rv;
		p1 += d1;
		p2 += d2;
		num--;
	}
	return 0;
}

SHARED_EXPORT int stricmp_utf8_max(const char * p1,const char * p2,unsigned p1_bytes)
{
	return stricmp_utf8_ex(p1,p1_bytes,p2,-1);
}

namespace {
	typedef bool (*t_replace_test)(const char * src,const char * test,unsigned len);

	static bool replace_test_i(const char * src,const char * test,unsigned len)
	{
		return stricmp_utf8_max(src,test,len)==0;
	}

	static bool replace_test(const char * src,const char * test,unsigned len)
	{
		unsigned ptr;
		bool rv = true;
		for(ptr=0;ptr<len;ptr++)
		{
			if (src[ptr]!=test[ptr]) {rv = false; break;}
		}
		return rv;
	}
}

SHARED_EXPORT unsigned uReplaceStringAdd(string_base & out,const char * src,unsigned src_len,const char * s1,unsigned len1,const char * s2,unsigned len2,bool casesens)
{
	t_replace_test testfunc = casesens ? replace_test : replace_test_i;

	len1 = strlen_max(s1,len1), len2 = strlen_max(s2,len2);

	unsigned len = strlen_max(src,src_len);
	
	unsigned count = 0;

	if (len1>0)
	{
		unsigned ptr = 0;
		while(ptr+len1<=len)
		{
			if (testfunc(src+ptr,s1,len1))
			{
				count++;
				out.add_string(s2,len2);
				ptr += len1;
			}
			else out.add_byte(src[ptr++]);
		}
		if (ptr<len) out.add_string(src+ptr,len-ptr);
	}
	return count;
}

SHARED_EXPORT unsigned uReplaceCharAdd(string_base & out,const char * src,unsigned src_len,unsigned c1,unsigned c2,bool casesens)
{
	assert(c1>0);
	assert(c2>0);
	char s1[8],s2[8];
	unsigned len1,len2;
	len1 = utf8_encode_char(c1,s1);
	len2 = utf8_encode_char(c2,s2);
	return uReplaceString(out,src,src_len,s1,len1,s2,len2,casesens);
}


SHARED_EXPORT void uAddStringLower(string_base & out,const char * src,unsigned len)
{
	while(*src && len)
	{
		unsigned int c,d;
		d = utf8_decode_char(src,&c);
		if (d==0 || d>len) break;
		out.add_char(char_lower(c));
		src+=d;
		len-=d;
	}
}

SHARED_EXPORT void uAddStringUpper(string_base & out,const char * src,unsigned len)
{
	while(*src && len)
	{
		unsigned int c,d;
		d = utf8_decode_char(src,&c);
		if (d==0 || d>len) break;
		out.add_char(char_upper(c));
		src+=d;
		len-=d;
	}
}

SHARED_EXPORT int stricmp_utf8_ex(const char * p1,unsigned p1_bytes,const char * p2,unsigned p2_bytes)
{
	p1_bytes = strlen_max(p1,p1_bytes);
	p2_bytes = strlen_max(p2,p2_bytes);
	for(;;)
	{
		if (p1_bytes == 0 && p2_bytes == 0) return 0;
		else if (p1_bytes == 0) return -1;
		else if (p2_bytes == 0) return 1;
		else if (*p1>0 && *p2>0)//signed char
		{
			unsigned c1 = q_tolower(*p1), c2 = q_tolower(*p2);
			if (c1<c2) return -1;
			else if (c1>c2) return 1;
			else
			{
				p1++;
				p2++;
				p1_bytes--;
				p2_bytes--;				
			}
		}
		else
		{
			unsigned w1,w2,d1,d2;
			d1 = utf8_decode_char(p1,&w1,p1_bytes);
			d2 = utf8_decode_char(p2,&w2,p2_bytes);
			if (d1==0) return -1;
			if (d2==0) return 1;
			int rv = compare_wchar(w1,w2);
			if (rv) return rv;
			p1 += d1;
			p2 += d2;
			p1_bytes -= d1;
			p2_bytes -= d2;
		}
	}
}

}
