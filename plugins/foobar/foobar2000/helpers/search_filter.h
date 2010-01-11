namespace search_tools {


		

	class NOVTABLE filter_node
	{
	public:
		virtual bool test(const file_info * item,const metadb_handle_ptr & handle) const = 0;
		virtual ~filter_node() {}
		static filter_node * create(const char * ptr,unsigned len,bool b_allow_simple = false);
		static filter_node * create(const class parser & p,bool b_allow_simple = false);
		
	};

	class search_filter
	{
		pfc::ptrholder_t<filter_node> m_root;

	public:
		search_filter() {}
		~search_filter() {}

		bool init(const char * pattern)
		{
			m_root =  filter_node::create(pattern,strlen(pattern),true) ;
			return m_root.is_valid();
		}

		void deinit()
		{
			m_root.release();
		}


		bool test(const file_info * item,const metadb_handle_ptr & handle) const
		{
			return m_root.is_valid() ? m_root->test(item,handle) : false;
		}

		bool test(const metadb_handle_ptr & item) const
		{
			bool rv = false;
			const file_info * ptr;
			if (item->get_info_locked(ptr)) rv = test(ptr,item);
			return rv;
		}
	};


};