#ifndef _APP_CLOSE_BLOCKER_H_
#define _APP_CLOSE_BLOCKER_H_


//! This service is used to signal whether something is currently preventing main window from being closed and app from being shut down.

class NOVTABLE app_close_blocker : public service_base
{
public:
	//! Checks whether this service is currently preventing main window from being closed and app from being shut down.
	virtual bool query() = 0;

	//! Static helper function, checks whether any of registered app_close_blocker services is currently preventing main window from being closed and app from being shut down.
	static bool g_query();

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	app_close_blocker() {}
	~app_close_blocker() {}
};

#endif //_APP_CLOSE_BLOCKER_H_