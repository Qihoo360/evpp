#include "evpp/inner_pre.h"

#include "evpp/libevent_headers.h"

#ifdef H_WINDOWS_API
    #pragma comment(lib,"libevent.lib")
    #pragma comment(lib,"Ws2_32.lib")
    #pragma comment(lib,"libglog_static.lib")
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

