#include "pfc.h"

bool string_base::add_char(unsigned c)
{
	char temp[8];
	unsigned len = utf8_encode_char(c,temp);
	if (len>0) return add_string(temp,len);
	else return true;
}

void string_base::skip_trailing_char(unsigned skip)
{
	const char * str = get_ptr();
	unsigned ptr,trunc;
	bool need_trunc = false;
	for(ptr=0;str[ptr];)
	{
		unsigned c;
		unsigned delta = utf8_decode_char(str+ptr,&c);
		if (delta==0) break;
		if (c==skip)
		{
			need_trunc = true;
			trunc = ptr;
		}
		else
		{
			need_trunc = false;
		}
		ptr += delta;
	}
	if (need_trunc) truncate(trunc);
}

format_time::format_time(t_int64 length)
{
	if (length<0) length=0;
	char * out = buffer;
	int weeks,days,hours,minutes,seconds;
	
	weeks = (int)( ( length / (60*60*24*7) ) );
	days = (int)( ( length / (60*60*24) ) % 7 );
	hours = (int) ( ( length / (60 * 60) ) % 24);
	minutes = (int) ( ( length / (60 ) ) % 60 );
	seconds = (int) ( ( length ) % 60 );

	if (weeks)
	{
		out += sprintf(out,"%uwk ",weeks);
	}
	if (days || weeks)
	{
		out += sprintf(out,"%ud ",days);
	}
	if (hours || days || weeks)
	{
		out += sprintf(out,"%u:%02u:%02u",hours,minutes,seconds);
	}
	else out += sprintf(out,"%u:%02u",minutes,seconds);
}

int strcmp_partial(const char * s1,const char * s2)
{
	while(*s2)
	{
		if (*s1<*s2) return -1;
		else if (*s1>*s2) return 1;
		s1++;
		s2++;
	}
	return 0;
}

bool string_base::add_float(double val,unsigned digits)
{
	char temp[_CVTBUFSIZE];
	_gcvt(val,digits,temp);
	return add_string(temp);
}

bool string_base::add_int(t_int64 val,unsigned base)
{
	char temp[64];
	_i64toa(val,temp,base);
	return add_string(temp);
}

bool string_base::add_uint(t_uint64 val,unsigned base)
{
	char temp[64];
	_ui64toa(val,temp,base);
	return add_string(temp);
}

bool is_path_separator(unsigned c)
{
	return c=='\\' || c=='/' || c=='|' || c==':';
}

bool is_path_bad_char(unsigned c)
{
#ifdef _WINDOWS
	return c=='\\' || c=='/' || c=='|' || c==':' || c=='*' || c=='?' || c=='\"' || c=='>' || c=='<';
#else
#error portme
#endif
}


void string_printf::g_run(string_base & out,const char * fmt,va_list list)
{
	out.reset();
	while(*fmt)
	{
		if (*fmt=='%')
		{
			fmt++;
			if (*fmt=='%')
			{
				out.add_char('%');
				fmt++;
			}
			else
			{
				bool force_sign = false;
				if (*fmt=='+')
				{
					force_sign = true;
					fmt++;
				}
				char padchar = (*fmt == '0') ? '0' : ' ';
				int pad = 0;
				while(*fmt>='0' && *fmt<='9')
				{
					pad = pad * 10 + (*fmt - '0');
					fmt++;
				}

				if (*fmt=='s' || *fmt=='S')
				{
					const char * ptr = va_arg(list,const char*);
					int len = strlen(ptr);
					if (pad>len) out.add_chars(padchar,pad-len);
					out.add_string(ptr);
					fmt++;

				}
				else if (*fmt=='i' || *fmt=='I' || *fmt=='d' || *fmt=='D')
				{
					char temp[8*sizeof(int)];
					int val = va_arg(list,int);
					if (force_sign && val>0) out.add_char('+');
					itoa(val,temp,10);
					int len = strlen(temp);
					if (pad>len) out.add_chars(padchar,pad-len);
					out.add_string(temp);
					fmt++;
				}
				else if (*fmt=='u' || *fmt=='U')
				{
					char temp[8*sizeof(int)];
					int val = va_arg(list,int);
					if (force_sign && val>0) out.add_char('+');
					_ultoa(val,temp,10);
					int len = strlen(temp);
					if (pad>len) out.add_chars(padchar,pad-len);
					out.add_string(temp);
					fmt++;
				}
				else if (*fmt=='x' || *fmt=='X')
				{
					char temp[8*sizeof(int)];
					int val = va_arg(list,int);
					if (force_sign && val>0) out.add_char('+');
					_ultoa(val,temp,16);
					if (*fmt=='X')
					{
						char * t = temp;
						while(*t)
						{
							if (*t>='a' && *t<='z')
								*t += 'A' - 'a';
							t++;
						}
					}
					int len = strlen(temp);
					if (pad>len) out.add_chars(padchar,pad-len);
					out.add_string(temp);
					fmt++;
				}
				else if (*fmt=='c' || *fmt=='C')
				{
					out.add_char(va_arg(list,char));
					fmt++;
				}
			}
		}
		else
		{
			out.add_char(*(fmt++));
		}
	}
}

string_printf::string_printf(const char * fmt,...)
{
	va_list list;
	va_start(list,fmt);
	run(fmt,list);
	va_end(list);
}



unsigned strlen_max(const char * ptr,unsigned max)
{
	if (ptr==0) return 0;
	unsigned int n = 0;
	while(n<max && ptr[n]) n++;
	return n;
}

unsigned wcslen_max(const WCHAR * ptr,unsigned max)
{
	if (ptr==0) return 0;
	unsigned int n = 0;
	while(n<max && ptr[n]) n++;
	return n;
}

char * strdup_n(const char * src,unsigned len)
{
	len = strlen_max(src,len);
	char * ret = (char*)malloc(len+1);
	if (ret)
	{
		memcpy(ret,src,len);
		ret[len]=0;
	}
	return ret;
}

bool string8::add_string(const char * ptr,unsigned len)
{
	if (len > 0 && ptr >= data.get_ptr() && ptr <= data.get_ptr() + data.get_size())
	{
		string8 temp;
		if (!temp.set_string(ptr,len)) return false;
		return add_string(temp);
	}
	else
	{
		len = strlen_max(ptr,len);
		if (!makespace(used+len+1)) return false;
		data.copy(ptr,len,used);
		used+=len;
		data[used]=0;
		return true;
	}
}

bool string8::set_string(const char * ptr,unsigned len)
{
	if (len > 0 && ptr >= data.get_ptr() && ptr < data.get_ptr() + data.get_size())
	{
		string8 temp;
		if (!temp.set_string(ptr,len)) return false;
		return set_string(temp);
	}
	else
	{
		len = strlen_max(ptr,len);
		if (!makespace(len+1)) return false;
		data.copy(ptr,len);
		used=len;
		data[used]=0;
		return true;
	}
}




void string8::set_char(unsigned offset,char c)
{
	if (!c) truncate(offset);
	else if (offset<used) data[offset]=c;
}

int string8::find_first(char c,int start)	//return -1 if not found
{
	unsigned n;
	if (start<0) start = 0;
	for(n=start;n<used;n++)
	{
		if (data[n]==c) return n;
	}
	return -1;
}

int string8::find_last(char c,int start)
{
	int n;
	if (start<0) start = used-1;
	for(n=start;n>=0;n--)
	{
		if (data[n]==c) return n;
	}
	return -1;
}

int string8::find_first(const char * str,int start)
{
	unsigned n;
	if (start<0) start=0;
	for(n=start;n<used;n++)
	{
		if (!strcmp_partial(data.get_ptr()+n,str)) return n;
	}
	return -1;
}

int string8::find_last(const char * str,int start)
{
	int n;
	if (start<0) start = used-1;
	for(n=start;n>=0;n--)
	{
		if (!strcmp_partial(data.get_ptr()+n,str)) return n;
	}
	return -1;
}

unsigned string8::g_scan_filename(const char * ptr)
{
	int n;
	int _used = strlen(ptr);
	for(n=_used-1;n>=0;n--)
	{
		if (is_path_separator(ptr[n])) return n+1;
	}
	return 0;
}

unsigned string8::scan_filename()
{
	int n;
	for(n=used-1;n>=0;n--)
	{
		if (is_path_separator(data[n])) return n+1;
	}
	return 0;
}

void string8::fix_filename_chars(char def,char leave)//replace "bad" characters, leave can be used to keep eg. path separators
{
	unsigned n;
	for(n=0;n<used;n++)
		if (data[n]!=leave && is_path_bad_char(data[n])) data[n]=def;
}

void string8::fix_dir_separator(char c)
{
	if (used==0 || data[used-1]!=c) add_char(c);
}

void string8::_xor(char x)//renamed from "xor" to keep weird compilers happy
{
	unsigned n;
	for(n=0;n<used;n++)
		data[n]^=x;
}


//slow
void string8::remove_chars(unsigned first,unsigned count)
{
	if (first>used) first = used;
	if (first+count>used) count = used-first;
	if (count>0)
	{
		unsigned n;
		for(n=first+count;n<=used;n++)
			data[n-count]=data[n];
		used -= count;
		makespace(used+1);
	}
}

//slow
void string8::insert_chars(unsigned first,const char * src, unsigned count)
{
	if (first > used) first = used;

	makespace(used+count+1);
	unsigned n;
	for(n=used;(int)n>=(int)first;n--)
		data[n+count] = data[n];
	for(n=0;n<count;n++)
		data[first+n] = src[n];

	used+=count;
}

void string8::insert_chars(unsigned first,const char * src) {insert_chars(first,src,strlen(src));}

bool string8::truncate_eol(unsigned start)
{
	unsigned n;
	const char * ptr = data + start;
	for(n=start;n<used;n++)
	{
		if (*ptr==10 || *ptr==13)
		{
			truncate(n);
			return true;
		}
		ptr++;
	}
	return false;
}

bool string8::fix_eol(const char * append,unsigned start)
{
	bool rv = truncate_eol(start);
	if (rv) add_string(append);
	return rv;
}

string_filename::string_filename(const char * fn)
{
	fn += string8::g_scan_filename(fn);
	const char * ptr=fn,*dot=0;
	while(*ptr && *ptr!='?')
	{
		if (*ptr=='.') dot=ptr;
		ptr++;
	}

	if (dot && dot>fn) set_string(fn,dot-fn);
	else set_string(fn);
}

string_filename_ext::string_filename_ext(const char * fn)
{
	fn += string8::g_scan_filename(fn);
	const char * ptr = fn;
	while(*ptr && *ptr!='?') ptr++;
	set_string(fn,ptr-fn);
}

string_extension::string_extension(const char * src)
{
	buffer[0]=0;
	const char * start = src + string8::g_scan_filename(src);
	const char * end = start + strlen(start);
	const char * ptr = end-1;
	while(ptr>start && *ptr!='.')
	{
		if (*ptr=='?') end=ptr;
		ptr--;
	}

	if (ptr>=start && *ptr=='.')
	{
		ptr++;
		unsigned len = end-ptr;
		if (len<tabsize(buffer))
		{
			memcpy(buffer,ptr,len*sizeof(char));
			buffer[len]=0;
		}
	}
}


bool has_path_bad_chars(const char * param)
{
	while(*param)
	{
		if (is_path_bad_char(*param)) return true;
		param++;
	}
	return false;
}

void pfc_float_to_string(char * out,unsigned out_max,double val,unsigned precision,bool b_sign)
{
	char temp[64];
	unsigned outptr;
	unsigned temp_len;

	if (out_max == 0) return;
	out_max--;//for null terminator
	
	outptr = 0;	

	if (outptr == out_max) {out[outptr]=0;return;}

	if (val<0) {out[outptr++] = '-'; val = -val;}
	else if (b_sign) {out[outptr++] = '+';}

	if (outptr == out_max) {out[outptr]=0;return;}

	
	{
		double powval = pow((double)10.0,(double)precision);
		t_int64 blargh = (t_int64)floor(val * powval + 0.5);
		_i64toa(blargh,temp,10);
	}
	
	temp_len = strlen(temp);
	if (temp_len <= precision)
	{
		out[outptr++] = '0';
		if (outptr == out_max) {out[outptr]=0;return;}
		out[outptr++] = '.';
		if (outptr == out_max) {out[outptr]=0;return;}
		unsigned d;
		for(d=precision-temp_len;d;d--)
		{
			out[outptr++] = '0';
			if (outptr == out_max) {out[outptr]=0;return;}
		}
		for(d=0;d<temp_len;d++)
		{
			out[outptr++] = temp[d];
			if (outptr == out_max) {out[outptr]=0;return;}
		}
	}
	else
	{
		unsigned d = temp_len;
		const char * src = temp;
		while(*src)
		{
			if (d-- == precision)
			{
				out[outptr++] = '.';
				if (outptr == out_max) {out[outptr]=0;return;}
			}
			out[outptr++] = *(src++);
			if (outptr == out_max) {out[outptr]=0;return;}
		}
	}
	out[outptr] = 0;
}

static double pfc_string_to_float_internal(const char * src)
{
	bool neg = false;
	t_int64 val = 0;
	int div = 0;
	bool got_dot = false;

	while(*src==' ') src++;

	if (*src=='-') {neg = true;src++;}
	else if (*src=='+') src++;
	
	while(*src)
	{
		if (*src>='0' && *src<='9')
		{
			int d = *src - '0';
			val = val * 10 + d;
			if (got_dot) div--;
			src++;
		}
		else if (*src=='.' || *src==',')
		{
			if (got_dot) break;
			got_dot = true;
			src++;
		}
		else if (*src=='E' || *src=='e')
		{
			src++;
			div += atoi(src);
			break;
		}
		else break;
	}
	if (neg) val = -val;
	return (double) val * pow(10.0,(double)div);
}

double pfc_string_to_float(const char * src,unsigned max)
{
	char blargh[128];
	strncpy(blargh,src,max>127?127:max);
	blargh[127]=0;
	return pfc_string_to_float_internal(blargh);
}

bool string8::limit_length(unsigned length_in_chars,const char * append)
{
	bool rv = false;
	const char * base = get_ptr(), * ptr = base;
	while(length_in_chars && utf8_advance(ptr)) length_in_chars--;
	if (length_in_chars==0)
	{
		truncate(ptr-base);
		add_string(append);
		rv = true;
	}
	return rv;
}

void string_base::convert_to_lower_ascii(const char * src,char replace)
{
	reset();
	assert(replace>0);
	while(*src)
	{
		unsigned c;
		unsigned delta = utf8_decode_char(src,&c);
		if (delta==0) {c = replace; delta = 1;}
		else if (c>=0x80) c = replace;
		add_byte((char)c);
		src += delta;
	}
}

void convert_to_lower_ascii(const char * src,unsigned max,char * out,char replace)
{
	unsigned ptr = 0;
	assert(replace>0);
	while(ptr<max && src[ptr])
	{
		unsigned c;
		unsigned delta = utf8_decode_char(src+ptr,&c,max-ptr);
		if (delta==0) {c = replace; delta = 1;}
		else if (c>=0x80) c = replace;
		*(out++) = (char)c;
		ptr += delta;
	}
	*out = 0;
}

string_list_nulldelimited::string_list_nulldelimited()
{
	reset();
}

void string_list_nulldelimited::add_string(const char * src)
{
	unsigned src_len = strlen(src) + 1;
	data.copy(src,src_len,len);
	len += src_len;
	data.copy("",1,len);
}

void string_list_nulldelimited::add_string_multi(const char * src)
{
	unsigned src_len = 0;
	while(src[src_len])
	{
		src_len += strlen(src+src_len) + 1;
	}
	data.copy(src,src_len,len);
	len += src_len;
	data.copy("",1,len);
}


void string_list_nulldelimited::reset()
{
	len = 0;
	data.copy("\0",2,len);
}

unsigned strstr_ex(const char * p_string,unsigned p_string_len,const char * p_substring,unsigned p_substring_len)
{
	p_string_len = strlen_max(p_string,p_string_len);
	p_substring_len = strlen_max(p_substring,p_substring_len);
	unsigned index = 0;
	while(index + p_substring_len <= p_string_len)
	{
		if (memcmp(p_string+index,p_substring,p_substring_len) == 0) return index;
		unsigned delta = utf8_char_len(p_string+index,p_string_len - index);
		if (delta == 0) break;
		index += delta;
	}
	return (unsigned)(-1);
}

unsigned atoui_ex(const char * p_string,unsigned p_string_len)
{
	unsigned ret = 0, ptr = 0;
	while(ptr<p_string_len)
	{
		char c = p_string[ptr];
		if (! ( c >= '0' && c <= '9' ) ) break;
		ret = ret * 10 + (unsigned)( c - '0' );
		ptr++;
	}
	return ret;
}

int strcmp_ex(const char* p1,unsigned n1,const char* p2,unsigned n2)
{
	unsigned idx = 0;
	n1 = strlen_max(p1,n1); n2 = strlen_max(p2,n2);
	for(;;)
	{
		if (idx == n1 && idx == n2) return 0;
		else if (idx == n1) return -1;//end of param1
		else if (idx == n2) return 1;//end of param2

		char c1 = p1[idx], c2 = p2[idx];
		if (c1<c2) return -1;
		else if (c1>c2) return 1;
		
		idx++;
	}
}

t_int64 atoi64_ex(const char * src,unsigned len)
{
	len = strlen_max(src,infinite);
	t_int64 ret = 0, mul = 1;
	unsigned ptr = len;
	unsigned start = 0;
	bool neg = false;
//	start += skip_spacing(src+start,len-start);
	if (start < len && src[start] == '-') {neg = true; start++;}
//	start += skip_spacing(src+start,len-start);
	
	while(ptr>start)
	{
		char c = src[--ptr];
		if (c>='0' && c<='9')
		{
			ret += (c-'0') * mul;
			mul *= 10;
		}
		else
		{
			ret = 0;
			mul = 1;
		}
	}
	return neg ? -ret : ret;
}

static inline char q_tolower(char c)
{
	if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
	return c;
}

int stricmp_ascii(const char * s1,const char * s2)
{
	for(;;)
	{
		char c1 = q_tolower(*s1), c2 = q_tolower(*s2);
		if (c1<c2) return -1;
		else if (c1>c2) return 1;
		else if (c1 == 0) return 0;
		s1++;
		s2++;
	}
}

unsigned string8::replace_nontext_chars(char p_replace)
{
	unsigned ret = 0;
	for(unsigned n=0;n<used;n++)
	{
		if ((unsigned char)data[n] < 32) {data[n] = p_replace; ret++; }
	}
	return ret;
}

unsigned string8::replace_byte(char c1,char c2,unsigned start)
{
	assert(c1 != 0); assert(c2 != 0);
	unsigned n, ret = 0;
	for(n=start;n<used;n++)
	{
		if (data[n] == c1) {data[n] = c2; ret++;}
	}
	return ret;
}

format_float::format_float(double p_val,unsigned p_width,unsigned p_prec)
{
	char temp[64];
	pfc_float_to_string(temp,64,p_val,p_prec,false);
	temp[63] = 0;
	unsigned len = strlen(temp);
	if (len < p_width)
		m_buffer.add_chars(' ',p_width-len);
	m_buffer += temp;
}

static char format_hex_char(unsigned p_val)
{
	assert(p_val < 16);
	return (p_val < 10) ? p_val + '0' : p_val - 10 + 'A';
}

format_hex::format_hex(t_uint64 p_val,unsigned p_width)
{
	if (p_width > 16) p_width = 16;
	else if (p_width == 0) p_width = 1;
	char temp[16];
	unsigned n;
	for(n=0;n<16;n++)
	{
		temp[15-n] = format_hex_char((unsigned)(p_val & 0xF));
		p_val >>= 4;
	}

	for(n=0;n<16 && temp[n] == '0';n++) {}
	
	if (n > 16 - p_width) n = 16 - p_width;
	
	char * out = m_buffer;
	for(;n<16;n++)
		*(out++) = temp[n];
	*out = 0;
}

format_uint::format_uint(t_uint64 val,unsigned p_width,unsigned p_base)
{
	
	enum {max_width = tabsize(m_buffer) - 1};

	if (p_width > max_width) p_width = max_width;
	else if (p_width == 0) p_width = 1;

	char temp[max_width];
	
	unsigned n;
	for(n=0;n<max_width;n++)
	{
		temp[max_width-1-n] = format_hex_char((unsigned)(val % p_base));
		val /= p_base;
	}

	for(n=0;n<max_width && temp[n] == '0';n++) {}
	
	if (n > max_width - p_width) n = max_width - p_width;
	
	char * out = m_buffer;

	for(;n<max_width;n++)
		*(out++) = temp[n];
	*out = 0;
}

format_fixedpoint::format_fixedpoint(t_int64 p_val,unsigned p_point)
{
	unsigned div = 1;
	for(unsigned n=0;n<p_point;n++) div *= 10;

	if (p_val < 0) {m_buffer << "-";p_val = -p_val;}

	
	m_buffer << format_int(p_val / div) << "." << format_int(p_val % div, p_point);
}

format_int::format_int(t_int64 p_val,unsigned p_width,unsigned p_base)
{
	bool neg = false;
	t_uint64 val;
	if (p_val < 0) {neg = true; val = (t_uint64)(-p_val);}
	else val = (t_uint64)p_val;
	
	enum {max_width = tabsize(m_buffer) - 1};

	if (p_width > max_width) p_width = max_width;
	else if (p_width == 0) p_width = 1;

	if (neg && p_width > 1) p_width --;
	
	char temp[max_width];
	
	unsigned n;
	for(n=0;n<max_width;n++)
	{
		temp[max_width-1-n] = format_hex_char((unsigned)(val % p_base));
		val /= p_base;
	}

	for(n=0;n<max_width && temp[n] == '0';n++) {}
	
	if (n > max_width - p_width) n = max_width - p_width;
	
	char * out = m_buffer;

	if (neg) *(out++) = '-';

	for(;n<max_width;n++)
		*(out++) = temp[n];
	*out = 0;
}


format_hexdump::format_hexdump(const void * p_buffer,unsigned p_bytes,const char * p_spacing)
{
	unsigned n;
	const t_uint8 * buffer = (const t_uint8*)p_buffer;
	for(n=0;n<p_bytes;n++)
	{
		if (n > 0 && p_spacing != 0) m_formatter << p_spacing;
		m_formatter << format_uint(buffer[n],2,16);
	}
}



string_replace_extension::string_replace_extension(const char * p_path,const char * p_ext)
{
	m_data = p_path;
	unsigned dot = m_data.find_last('.');
	if (dot < m_data.scan_filename())
	{//argh
		m_data += ".";
		m_data += p_ext;
	}
	else
	{
		m_data.truncate(dot+1);
		m_data += p_ext;
	}
}

string_directory::string_directory(const char * p_path)
{
	unsigned ptr = string8::g_scan_filename(p_path);
	if (ptr > 0) m_data.set_string(p_path,ptr-1);
}





void string_base::add_string_e(const char * p_string,unsigned p_length) {
	if (!add_string(p_string,p_length)) throw std::bad_alloc();
}
void string_base::set_string_e(const char * p_string,unsigned p_length) {
	if (!set_string(p_string,p_length)) throw std::bad_alloc();
}