#if 1
namespace pfc {

	namespace stringcvt {

#ifdef _WINDOWS
		enum {
			codepage_system = CP_ACP,
			codepage_ascii = 20127,
			codepage_iso_8859_1 = 28591,
		};
#else
#error portme
#endif

		//! Converts UTF-8 characters to wide character.
		//! @param p_out Output buffer, receives converted string, with null terminator.
		//! @param p_out_size Size of output buffer, in characters. If converted string is too long, it will be truncated. Null terminator is always written, unless p_out_size is zero.
		//! @param p_source String to convert.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters written, not counting null terminator.
		unsigned convert_utf8_to_wide(wchar_t * p_out,unsigned p_out_size,const char * p_source,unsigned p_source_size);

		//! Estimates buffer size required to convert specified UTF-8 string to widechar.
		//! @param p_source String to be converted.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters to allocate, including space for null terminator.
		unsigned estimate_utf8_to_wide(const char * p_source,unsigned p_source_size);

		//! Converts wide character string to UTF-8.
		//! @param p_out Output buffer, receives converted string, with null terminator.
		//! @param p_out_size Size of output buffer, in characters. If converted string is too long, it will be truncated. Null terminator is always written, unless p_out_size is zero.
		//! @param p_source String to convert.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters written, not counting null terminator.
		unsigned convert_wide_to_utf8(char * p_out,unsigned p_out_size,const wchar_t * p_source,unsigned p_source_size);

		//! Estimates buffer size required to convert specified wide character string to UTF-8.
		//! @param p_source String to be converted.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters to allocate, including space for null terminator.
		unsigned estimate_wide_to_utf8(const wchar_t * p_source,unsigned p_source_size);



		//! Converts string from specified codepage to wide character.
		//! @param p_out Output buffer, receives converted string, with null terminator.
		//! @param p_codepage Codepage ID of source string.
		//! @param p_source String to convert.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @param p_out_size Size of output buffer, in characters. If converted string is too long, it will be truncated. Null terminator is always written, unless p_out_size is zero.
		//! @returns Number of characters written, not counting null terminator.
		unsigned convert_codepage_to_wide(unsigned p_codepage,wchar_t * p_out,unsigned p_out_size,const char * p_source,unsigned p_source_size);

		//! Estimates buffer size required to convert specified string from specified codepage to wide character.
		//! @param p_codepage Codepage ID of source string.
		//! @param p_source String to be converted.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters to allocate, including space for null terminator.
		unsigned estimate_codepage_to_wide(unsigned p_codepage,const char * p_source,unsigned p_source_size);

		//! Converts string from wide character to specified codepage.
		//! @param p_codepage Codepage ID of source string.
		//! @param p_out Output buffer, receives converted string, with null terminator.
		//! @param p_out_size Size of output buffer, in characters. If converted string is too long, it will be truncated. Null terminator is always written, unless p_out_size is zero.
		//! @param p_source String to convert.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters written, not counting null terminator.
		unsigned convert_wide_to_codepage(unsigned p_codepage,char * p_out,unsigned p_out_size,const wchar_t * p_source,unsigned p_source_size);

		//! Estimates buffer size required to convert specified wide character string to specified codepage.
		//! @param p_codepage Codepage ID of source string.
		//! @param p_source String to be converted.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters to allocate, including space for null terminator.
		unsigned estimate_wide_to_codepage(unsigned p_codepage,const wchar_t * p_source,unsigned p_source_size);
		

		//! Converts string from system codepage to wide character.
		//! @param p_out Output buffer, receives converted string, with null terminator.
		//! @param p_source String to convert.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @param p_out_size Size of output buffer, in characters. If converted string is too long, it will be truncated. Null terminator is always written, unless p_out_size is zero.
		//! @returns Number of characters written, not counting null terminator.
		inline unsigned convert_ansi_to_wide(wchar_t * p_out,unsigned p_out_size,const char * p_source,unsigned p_source_size) {
			return convert_codepage_to_wide(codepage_system,p_out,p_out_size,p_source,p_source_size);
		}

		//! Estimates buffer size required to convert specified system codepage string to wide character.
		//! @param p_source String to be converted.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters to allocate, including space for null terminator.
		inline unsigned estimate_ansi_to_wide(const char * p_source,unsigned p_source_size) {
			return estimate_codepage_to_wide(codepage_system,p_source,p_source_size);
		}

		//! Converts string from wide character to system codepage.
		//! @param p_out Output buffer, receives converted string, with null terminator.
		//! @param p_out_size Size of output buffer, in characters. If converted string is too long, it will be truncated. Null terminator is always written, unless p_out_size is zero.
		//! @param p_source String to convert.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters written, not counting null terminator.
		inline unsigned convert_wide_to_ansi(char * p_out,unsigned p_out_size,const wchar_t * p_source,unsigned p_source_size) {
			return convert_wide_to_codepage(codepage_system,p_out,p_out_size,p_source,p_source_size);
		}

		//! Estimates buffer size required to convert specified wide character string to system codepage.
		//! @param p_source String to be converted.
		//! @param p_source_size Number of characters to read from p_source. If reading stops if null terminator is encountered earlier.
		//! @returns Number of characters to allocate, including space for null terminator.
		inline unsigned estimate_wide_to_ansi(const wchar_t * p_source,unsigned p_source_size) {
			return estimate_wide_to_codepage(codepage_system,p_source,p_source_size);
		}

		template<typename t_char> const t_char * null_string_t();
		template<> inline const char * null_string_t<char>() {return "";}
		template<> inline const wchar_t * null_string_t<wchar_t>() {return L"";}

		template<typename t_char> unsigned strlen_t(const t_char * p_string,unsigned p_string_size = infinite) {
			for(unsigned n=0;n<p_string_size;n++) {
				if (p_string[n] == 0) return n;
			}
			return p_string_size;
		}

		template<typename t_char> bool string_is_empty_t(const t_char * p_string,unsigned p_string_size = infinite) {
			if (p_string_size == 0) return true;
			return p_string[0] == 0;
		}

		template<typename t_char> class char_buffer_t {
		public:
			char_buffer_t() {}
			char_buffer_t(const char_buffer_t & p_source) : m_buffer(p_source.m_buffer) {}
			bool set_size(unsigned p_count) {return m_buffer.set_size(p_count);}
			t_char * get_ptr_var() {return m_buffer.get_ptr();}
			const t_char * get_ptr() const {
				return m_buffer.get_size() > 0 ? m_buffer.get_ptr() : null_string_t<t_char>();
			}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			mem_block_t<t_char> m_buffer;
		};

		class string_utf8_from_wide {
		public:
			string_utf8_from_wide() {}
			string_utf8_from_wide(const wchar_t * p_source,unsigned p_source_size = infinite) {convert_e(p_source,p_source_size);}
		
			void convert_e(const wchar_t * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_source,p_source_size)) throw std::bad_alloc();
			}

			bool convert(const wchar_t * p_source,unsigned p_source_size = infinite) {
				unsigned size = estimate_wide_to_utf8(p_source,p_source_size);
				if (!m_buffer.set_size(size)) return false;
				convert_wide_to_utf8( m_buffer.get_ptr_var(),size,p_source,p_source_size);
				return true;
			}

			operator const char * () const {return get_ptr();}
			const char * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			char_buffer_t<char> m_buffer;
		};
		
		class string_wide_from_utf8 {
		public:
			string_wide_from_utf8() {}
			string_wide_from_utf8(const string_wide_from_utf8 & p_source) : m_buffer(p_source.m_buffer) {}
			string_wide_from_utf8(const char* p_source,unsigned p_source_size = infinite) {convert_e(p_source,p_source_size);}

			void convert_e(const char * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_source,p_source_size)) throw std::bad_alloc();
			}

			bool convert(const char* p_source,unsigned p_source_size = infinite) {
				unsigned size = estimate_utf8_to_wide(p_source,p_source_size);
				if (!m_buffer.set_size(size)) return false;
				convert_utf8_to_wide( m_buffer.get_ptr_var(),size,p_source,p_source_size);
				return true;
			}

			operator const wchar_t * () const {return get_ptr();}
			const wchar_t * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			char_buffer_t<wchar_t> m_buffer;
		};

		class string_wide_from_codepage {
		public:
			string_wide_from_codepage() {}
			string_wide_from_codepage(const string_wide_from_codepage & p_source) : m_buffer(p_source.m_buffer) {}
			string_wide_from_codepage(unsigned p_codepage,const char * p_source,unsigned p_source_size = infinite) {convert_e(p_codepage,p_source,p_source_size);}

			void convert_e(unsigned p_codepage,const char * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_codepage,p_source,p_source_size)) throw std::bad_alloc();
			}

			bool convert(unsigned p_codepage,const char * p_source,unsigned p_source_size = infinite) {
				unsigned size = estimate_codepage_to_wide(p_codepage,p_source,p_source_size);
				if (!m_buffer.set_size(size)) return false;
				convert_codepage_to_wide(p_codepage, m_buffer.get_ptr_var(),size,p_source,p_source_size);
				return true;
			}

			operator const wchar_t * () const {return get_ptr();}
			const wchar_t * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			char_buffer_t<wchar_t> m_buffer;
		};

		class string_codepage_from_wide {
		public:
			string_codepage_from_wide() {}
			string_codepage_from_wide(const string_codepage_from_wide & p_source) : m_buffer(p_source.m_buffer) {}
			string_codepage_from_wide(unsigned p_codepage,const wchar_t * p_source,unsigned p_source_size = infinite) {convert_e(p_codepage,p_source,p_source_size);}

			void convert_e(unsigned p_codepage,const wchar_t * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_codepage,p_source,p_source_size)) throw std::bad_alloc();
			}

			bool convert(unsigned p_codepage,const wchar_t * p_source,unsigned p_source_size = infinite) {
				unsigned size = estimate_wide_to_codepage(p_codepage,p_source,p_source_size);
				if (!m_buffer.set_size(size)) return false;
				convert_wide_to_codepage(p_codepage, m_buffer.get_ptr_var(),size,p_source,p_source_size);
				return true;
			}

			operator const char * () const {return get_ptr();}
			const char * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			char_buffer_t<char> m_buffer;
		};

		class string_codepage_from_utf8 {
		public:
			string_codepage_from_utf8() {}
			string_codepage_from_utf8(const string_codepage_from_utf8 & p_source) : m_buffer(p_source.m_buffer) {}
			string_codepage_from_utf8(unsigned p_codepage,const char * p_source,unsigned p_source_size = infinite) {convert_e(p_codepage,p_source,p_source_size);}
			
			void convert_e(unsigned p_codepage,const char * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_codepage,p_source,p_source_size)) throw std::bad_alloc();
			}
			
			bool convert(unsigned p_codepage,const char * p_source,unsigned p_source_size = infinite) {
				string_wide_from_utf8 temp;
				if (!temp.convert(p_source,p_source_size)) return false;
				unsigned size = estimate_wide_to_codepage(p_codepage,temp,infinite);
				if (!m_buffer.set_size(size)) return false;
				convert_wide_to_codepage(p_codepage,m_buffer.get_ptr_var(),size,temp,infinite);
				return true;
			}

			operator const char * () const {return get_ptr();}
			const char * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			char_buffer_t<char> m_buffer;
		};

		class string_utf8_from_codepage {
		public:
			string_utf8_from_codepage() {}
			string_utf8_from_codepage(const string_utf8_from_codepage & p_source) : m_buffer(p_source.m_buffer) {}
			string_utf8_from_codepage(unsigned p_codepage,const char * p_source,unsigned p_source_size = infinite) {convert_e(p_codepage,p_source,p_source_size);}
			
			void convert_e(unsigned p_codepage,const char * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_codepage,p_source,p_source_size)) throw std::bad_alloc();
			}

			bool convert(unsigned p_codepage,const char * p_source,unsigned p_source_size = infinite) {
				string_wide_from_codepage temp;
				if (!temp.convert(p_codepage,p_source,p_source_size)) return false;
				unsigned size = estimate_wide_to_utf8(temp,infinite);
				if (!m_buffer.set_size(size)) return false;
				convert_wide_to_utf8( m_buffer.get_ptr_var(),size,temp,infinite);
				return true;
			}

			operator const char * () const {return get_ptr();}
			const char * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			char_buffer_t<char> m_buffer;
		};


		class string_utf8_from_ansi {
		public:
			string_utf8_from_ansi() {}
			string_utf8_from_ansi(const string_utf8_from_ansi & p_source) : m_buffer(p_source.m_buffer) {}
			string_utf8_from_ansi(const char * p_source,unsigned p_source_size = infinite) : m_buffer(codepage_system,p_source,p_source_size) {}
			operator const char * () const {return get_ptr();}
			const char * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void convert_e(const char * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_source,p_source_size)) throw std::bad_alloc();
			}

			bool convert(const char * p_source,unsigned p_source_size = infinite) {return m_buffer.convert(codepage_system,p_source,p_source_size);}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			string_utf8_from_codepage m_buffer;
		};

		class string_ansi_from_utf8 {
		public:
			string_ansi_from_utf8() {}
			string_ansi_from_utf8(const string_ansi_from_utf8 & p_source) : m_buffer(p_source.m_buffer) {}
			string_ansi_from_utf8(const char * p_source,unsigned p_source_size = infinite) : m_buffer(codepage_system,p_source,p_source_size) {}
			operator const char * () const {return get_ptr();}
			const char * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void convert_e(const char * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_source,p_source_size)) throw std::bad_alloc();
			}

			bool convert(const char * p_source,unsigned p_source_size = infinite) {return m_buffer.convert(codepage_system,p_source,p_source_size);}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			string_codepage_from_utf8 m_buffer;
		};

		class string_wide_from_ansi {
		public:
			string_wide_from_ansi() {}
			string_wide_from_ansi(const string_wide_from_ansi & p_source) : m_buffer(p_source.m_buffer) {}
			string_wide_from_ansi(const char * p_source,unsigned p_source_size = infinite) : m_buffer(codepage_system,p_source,p_source_size) {}
			operator const wchar_t * () const {return get_ptr();}
			const wchar_t * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void convert_e(const char * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_source,p_source_size)) throw std::bad_alloc();
			}

			bool convert(const char * p_source,unsigned p_source_size = infinite) {return m_buffer.convert(codepage_system,p_source,p_source_size);}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			string_wide_from_codepage m_buffer;
		};

		class string_ansi_from_wide {
		public:
			string_ansi_from_wide() {}
			string_ansi_from_wide(const string_ansi_from_wide & p_source) : m_buffer(p_source.m_buffer) {}
			string_ansi_from_wide(const wchar_t * p_source,unsigned p_source_size = infinite) : m_buffer(codepage_system,p_source,p_source_size) {}
			operator const char * () const {return get_ptr();}
			const char * get_ptr() const {return m_buffer.get_ptr();}
			bool is_empty() const {return string_is_empty_t(get_ptr());}
			unsigned length() const {return strlen_t(get_ptr());}

			void convert_e(const wchar_t * p_source,unsigned p_source_size = infinite) {
				if (!convert(p_source,p_source_size)) throw std::bad_alloc();
			}

			bool convert(const wchar_t * p_source,unsigned p_source_size = infinite) {return m_buffer.convert(codepage_system,p_source,p_source_size);}

			void set_mem_logic(mem_block::t_mem_logic p_value) {m_buffer.set_mem_logic(p_value);}
		private:
			string_codepage_from_wide m_buffer;
		};

#ifdef UNICODE
		typedef string_wide_from_utf8 string_os_from_utf8;
		typedef string_utf8_from_wide string_utf8_from_os;
#else
		typedef string_ansi_from_utf8 string_os_from_utf8;
		typedef string_utf8_from_ansi string_utf8_from_os;
#endif
	}
};
#endif