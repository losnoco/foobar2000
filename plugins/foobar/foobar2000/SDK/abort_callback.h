#ifndef _foobar2000_sdk_abort_callback_h_
#define _foobar2000_sdk_abort_callback_h_

namespace foobar2000_io {

PFC_DECLARE_EXCEPTION(exception_aborted,pfc::exception,"User abort");

//! This class is used to signal underlying worker code whether user has decided to abort a potentially time-consuming operation. It is commonly required by all file related operations. Code that receives an abort_callback object should periodically check it and abort any operations being performed if it is signaled, typically giving io_result_aborted return code (see: t_io_result). \n
//! See abort_callback_impl for implementation.
class NOVTABLE abort_callback
{
public:
	//! Returns whether user has requested the operation to be aborted.
	virtual bool FB2KAPI is_aborting() = 0;
	
	//! Checks if user has requested the operation to be aborted, and throws exception_aborted if so.
	void check();

	//! For compatibility with old code.
	inline void check_e() {check();}
protected:
	abort_callback() {}
	~abort_callback() {}
};



//! Implementation of abort_callback interface.
class abort_callback_impl : public abort_callback
{
	bool m_aborting;
public:
	abort_callback_impl() : m_aborting(false) {}
	inline void abort() {m_aborting = true;}
	inline void reset() {m_aborting = false;}
	bool FB2KAPI is_aborting() {return m_aborting;}
};

}

using namespace foobar2000_io;

#endif //_foobar2000_sdk_abort_callback_h_