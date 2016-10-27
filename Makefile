
all : 
	$(MAKE) -C evpp
#	$(MAKE) -C 3rdparty
#	$(MAKE) -C apps

pkg :
	$(MAKE) -C rpm

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
	$(MAKE) clean -C evpp
	$(MAKE) clean -C test
	$(MAKE) clean -C examples
	$(MAKE) clean -C apps
	$(MAKE) clean -C 3rdparty


.PHONY: all test check clean apps 3rdparty pkg
