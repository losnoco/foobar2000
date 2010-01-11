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
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

};

#endif //_APP_CLOSE_BLOCKER_H_