#pragma once

#ifdef ENABLE_CUBISM3_DEBUG
	#ifdef ENABLE_CUBISM3_DEBUGLOG
		#define CLOG(...) CryLog(__VA_ARGS__)
	#else
		#define CLOG(...)
	#endif
	#ifdef ENABLE_CUBISM3_THREADLOG
		#define TLOG(...) CryLog(__VA_ARGS__)
	#else
		#define TLOG(...)
	#endif
#else
	#define CLOG(...)
	#define TLOG(...)
#endif