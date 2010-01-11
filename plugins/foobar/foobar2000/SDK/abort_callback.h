#ifndef _foobar2000_sdk_abort_callback_h_
#define _foobar2000_sdk_abort_callback_h_

//! This class is used to signal underlying worker code whether user has decided to abort a potentially time-consuming operation. It is commonly required by all file related operations. Code that receives an abort_callback object should periodically check it and abort any operations being performed if it is signaled, typically giving io_result_aborted return code (see: t_io_result). \n
//! See abort_callback_impl for implementation.
class NOVTABLE abort_callback
{
public:
	//! Returns whether user has requested the operation to be aborted.
	virtual bool is_aborting() = 0;
	
	//! Checks if user has requested the operation to be aborted, and throws io_result_aborted exception if so.
	void check_e();
};


//! Implementation of abort_callback interface.
class abort_callback_impl : public abort_callback
{
	bool m_aborting;
public:
	abort_callback_impl() : m_aborting(false) {}
	inline void abort() {m_aborting = true;}
	inline void reset() {m_aborting = false;}
	virtual bool is_aborting() {return m_aborting;}
};

#endif //_foobar2000_sdk_abort_callback_h_