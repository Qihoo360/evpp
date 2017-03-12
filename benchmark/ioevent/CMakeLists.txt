if (UNIX)
	add_subdirectory(libevent)
endif (UNIX)

add_subdirectory(evpp)
add_subdirectory(fd_channel_vs_pipe_event_watcher)
