#ifndef _INITQUIT_H_
#define _INITQUIT_H_

#include "service.h"

//init/quit callback, on_init is called after main window has been created, on_quit is called before main window is destroyed
class NOVTABLE initquit : public service_base
{
public:
	virtual void on_init() {}
	virtual void on_quit() {}

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	initquit() {}
	~initquit() {}
};

template<class T>
class initquit_factory_t : public service_factory_single_t<initquit,T> {};

#endif