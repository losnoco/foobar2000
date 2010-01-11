namespace pfc {

	class NOVTABLE bsearch_callback
	{
	public:
		virtual int test(unsigned p_index) const = 0;
	};

	PFC_DLL_EXPORT bool bsearch(unsigned p_count, bsearch_callback const & p_callback,unsigned & p_result);

	template<typename t_container,typename t_compare, typename t_param>
	class bsearch_callback_impl_simple_t : public bsearch_callback
	{
	public:
		int test(unsigned p_index) const
		{
			return m_compare(m_container[p_index],m_param);
		}
		bsearch_callback_impl_simple_t(const t_container & p_container,t_compare p_compare,t_param p_param)
			: m_container(p_container), m_compare(p_compare), m_param(p_param)
		{
		}

	private:
		const t_container & m_container;
		t_compare m_compare;
		t_param m_param;
	};


	template<typename t_container,typename t_compare, typename t_param>
	bool bsearch_t(unsigned p_count,const t_container & p_container,t_compare p_compare,t_param p_param,unsigned & index)
	{
		return bsearch(
			p_count,
			bsearch_callback_impl_simple_t<t_container,t_compare,t_param>(p_container,p_compare,p_param),
			index);
	}
}