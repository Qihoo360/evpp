
SUBDIRS = echo/tcpecho \
		  echo/httpecho \
		  pingpong/server \
		  pingpong/client \

all : 
	for t in $(SUBDIRS); do $(MAKE) -C $$t; done

clean:
	for t in $(SUBDIRS); do $(MAKE) clean -C $$t; done


.PHONY: all test check clean apps 3rdparty pkg fmt
