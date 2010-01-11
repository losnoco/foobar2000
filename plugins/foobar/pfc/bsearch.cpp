#include "pfc.h"



/*
class NOVTABLE bsearch_callback
{
public:
	virtual int test(unsigned p_index) const = 0;
};
*/

PFC_DLL_EXPORT bool pfc::bsearch(unsigned p_count, pfc::bsearch_callback const & p_callback,unsigned & p_result)
{
	return bsearch_inline_t<pfc::bsearch_callback const &>(p_count,p_callback,p_result);
}