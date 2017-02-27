
SUBDIRS = evpp test examples apps 3rdparty

all : 
	for t in $(SUBDIRS); do $(MAKE) -C $$t; done

test : all
	$(MAKE) -C test
	$(MAKE) -C examples

apps :
	$(MAKE) -C apps

3rdparty :
	$(MAKE) -C 3rdparty

check : all
	$(MAKE) check -C test

clean:
	for t in $(SUBDIRS); do $(MAKE) clean -C $$t; done

.PHONY: all test check clean apps 3rdparty pkg fmt
