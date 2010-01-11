#include "pfc.h"


namespace {
	template<typename t_char> 
	class string_writer_t {
	public:
		string_writer_t(t_char * p_buffer,unsigned p_size) : m_buffer(p_buffer), m_size(p_size), m_writeptr(0) {}
		
		void write(t_char p_char) {
			if (m_writeptr < m_size) {
				m_buffer[m_writeptr++] = p_char;
			}
		}
		void write_multi(const t_char * p_buffer,unsigned p_count) {
			const unsigned delta = pfc::min_t<unsigned>(p_count,m_size-m_writeptr);
			for(unsigned n=0;n<delta;n++) {
				m_buffer[m_writeptr++] = p_buffer[n];
			}
		}

		void write_as_utf8(unsigned p_char) {
			char temp[6];
			unsigned n = utf8_encode_char(p_char,temp);
			write_multi(temp,n);
		}

		void write_as_wide(unsigned p_char) {
			wchar_t temp[2];
			unsigned n = utf16_encode_char(p_char,temp);
			write_multi(temp,n);
		}

		unsigned finalize() {
			if (m_size == 0) return 0;
			unsigned terminator = pfc::min_t<unsigned>(m_writeptr,m_size-1);
			m_buffer[terminator] = 0;
			return terminator;
		}
		bool is_overrun() const {
			return m_writeptr >= m_size;
		}
	private:
		t_char * m_buffer;
		unsigned m_size;
		unsigned m_writeptr;
	};



};

namespace pfc {
	namespace stringcvt {


		unsigned convert_utf8_to_wide(wchar_t * p_out,unsigned p_out_size,const char * p_in,unsigned p_in_size) {
			const unsigned insize = p_in_size;
			unsigned inptr = 0;
			string_writer_t<wchar_t> writer(p_out,p_out_size);

			while(inptr < insize && !writer.is_overrun()) {
				unsigned newchar = 0;
				unsigned delta = utf8_decode_char(p_in + inptr,&newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				assert(inptr + delta <= insize);
				inptr += delta;
				writer.write_as_wide(newchar);
			}

			return writer.finalize();
		}

		unsigned convert_wide_to_utf8(char * p_out,unsigned p_out_size,const wchar_t * p_in,unsigned p_in_size) {
			const unsigned insize = p_in_size;
			unsigned inptr = 0;
			string_writer_t<char> writer(p_out,p_out_size);

			while(inptr < insize && !writer.is_overrun()) {
				unsigned newchar = 0;
				unsigned delta = utf16_decode_char(p_in + inptr,&newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				assert(inptr + delta <= insize);
				inptr += delta;
				writer.write_as_utf8(newchar);
			}

			return writer.finalize();
		}

		unsigned estimate_utf8_to_wide(const char * p_in,unsigned p_in_size) {
			const unsigned insize = p_in_size;
			unsigned inptr = 0;
			unsigned retval = 1;//1 for null terminator
			while(inptr < insize) {
				unsigned newchar = 0;
				unsigned delta = utf8_decode_char(p_in + inptr,&newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				assert(inptr + delta <= insize);
				inptr += delta;
				
				{
					wchar_t temp[2];
					delta = utf16_encode_char(newchar,temp);
					if (delta == 0) break;
					retval += delta;
				}
			}
			return retval;
		}

		unsigned estimate_wide_to_utf8(const wchar_t * p_in,unsigned p_in_size) {
			const unsigned insize = p_in_size;
			unsigned inptr = 0;
			unsigned retval = 1;//1 for null terminator
			while(inptr < insize) {
				unsigned newchar = 0;
				unsigned delta = utf16_decode_char(p_in + inptr,&newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				assert(inptr + delta <= insize);
				inptr += delta;
				
				{
					char temp[6];
					delta = utf8_encode_char(newchar,temp);
					if (delta == 0) break;
					retval += delta;
				}
			}
			return retval;
		}


		unsigned convert_codepage_to_wide(unsigned p_codepage,wchar_t * p_out,unsigned p_out_size,const char * p_source,unsigned p_source_size) {
			if (p_out_size == 0) return 0;
			memset(p_out,0,p_out_size * sizeof(*p_out));
			MultiByteToWideChar(p_codepage,0,p_source,p_source_size,p_out,p_out_size);
			p_out[p_out_size-1] = 0;
			return wcslen(p_out);
		}

		unsigned convert_wide_to_codepage(unsigned p_codepage,char * p_out,unsigned p_out_size,const wchar_t * p_source,unsigned p_source_size) {
			if (p_out_size == 0) return 0;
			memset(p_out,0,p_out_size * sizeof(*p_out));
			WideCharToMultiByte(p_codepage,0,p_source,p_source_size,p_out,p_out_size,0,FALSE);
			p_out[p_out_size-1] = 0;
			return strlen(p_out);
		}

		unsigned estimate_codepage_to_wide(unsigned p_codepage,const char * p_source,unsigned p_source_size) {
			return MultiByteToWideChar(p_codepage,0,p_source,strlen_max(p_source,p_source_size),0,0) + 1;
		}
		unsigned estimate_wide_to_codepage(unsigned p_codepage,const wchar_t * p_source,unsigned p_source_size) {
			return WideCharToMultiByte(p_codepage,0,p_source,wcslen_max(p_source,p_source_size),0,0,0,FALSE) + 1;
		}
	}

}