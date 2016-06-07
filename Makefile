
all : 
	$(MAKE) -C src/base
	$(MAKE) -C src
	$(MAKE) -C test

check : all
	$(MAKE) check -C test

clean:
	$(MAKE) clean -C src/base
	$(MAKE) clean -C src
	$(MAKE) clean -C test
