
all : 
	$(MAKE) -C evpp

test : all
	$(MAKE) -C test
	$(MAKE) -C examples

apps :
	$(MAKE) -C apps

check : all
	$(MAKE) check -C test

clean:
	$(MAKE) clean -C evpp
	$(MAKE) clean -C test
	$(MAKE) clean -C examples
	$(MAKE) clean -C apps


.PHONY: all test check clean apps
