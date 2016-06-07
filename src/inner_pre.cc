#include "evpp/inner_pre.h"

#include "evpp/libevent_headers.h"

#ifdef H_WINDOWS_API

#if _EVENT_NUMERIC_VERSION >= 0x02001500
#pragma comment(lib,"libevent.lib")
// #pragma comment(lib,"libevent_core.lib")
// #pragma comment(lib,"libevent_extras.lib")
#else
#	ifdef H_DEBUG_MODE
#		pragma comment(lib,"libevent_d.lib")
#	else
#		pragma comment(lib,"libevent.lib")
#	endif
#endif

#	pragma comment(lib,"Ws2_32.lib")
#	pragma comment(lib,"libglog_static.lib")
#endif


#ifdef H_OS_LINUX
#include <signal.h>
// set signal handler
void sig_pipe( int id )
{
	//H_LOG_NAME_DEBUG( "", "a pipe arrived.");
    // do nothing.
	//printf( "signal pipe:%d", id );
}
void sig_child( int )
{
    //H_LOG_NAME_DEBUG( "", "a child thread terminated." );
    // do nothing.
}

#endif

