
all : 
	$(MAKE) -C src

test : all
	$(MAKE) -C test
	$(MAKE) -C examples

check : all
	$(MAKE) check -C test

clean:
	$(MAKE) clean -C src
	$(MAKE) clean -C test
	$(MAKE) clean -C examples


.PHONY: all test check clean
