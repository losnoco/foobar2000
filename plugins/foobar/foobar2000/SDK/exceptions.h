#if FOOBAR2000_TARGET_VERSION >= 76

//! Base class for exceptions that should show a human readable message when caught in app entrypoint function rather than crash and send a crash report.
PFC_DECLARE_EXCEPTION(exception_messagebox,pfc::exception,"Internal Error");

//! Base class for exceptions that should result in a quiet app shutdown.
PFC_DECLARE_EXCEPTION(exception_shutdownrequest,pfc::exception,"Shutdown Request");


PFC_DECLARE_EXCEPTION(exception_installdamaged, exception_messagebox, "Internal Error - please re-run the foobar2000 installer.");
PFC_DECLARE_EXCEPTION(exception_out_of_resources, exception_messagebox, "Not enough system resources available.");

#endif