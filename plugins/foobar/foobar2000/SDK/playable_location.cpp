#include "foobar2000.h"

int playable_location::compare(const playable_location & src) const
{
	int ret = uStringCompare(get_path(),src.get_path());
	if (ret!=0) return ret;
	else
	{
		ret = metadb::path_compare(get_path(),src.get_path());
		if (ret != 0) return ret;
		else
		{
			int n1 = get_subsong(), n2 = src.get_subsong();
			if (n1<n2) return -1;
			else if (n1>n2) return 1;
			else return 0;
		}
	}
}

string_formatter & operator<<(string_formatter & p_fmt,const playable_location & p_location)
{
	p_fmt << "\"" << file_path_display(p_location.get_path()) << "\"";
	t_uint32 index = p_location.get_subsong_index();
	if (index != 0) p_fmt << " / index: " << p_location.get_subsong_index();
	return p_fmt;
}