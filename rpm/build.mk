
PROJ  = evpp

SRC			= $(PROJ).tar.gz
PKG 		= $(wildcard *.spec)
TEMP_DIR 	= /home/$(shell whoami)/tmp
ROOT_DIR 	= $(shell PWD=$$(pwd); echo $${PWD%%/$(PROJ)*}/$(PROJ))
COMMITVERSION  = $(shell git log --format=%H -n1 | cut -b 1-8 )
MAJOR := 1
MINOR := 0
FULL := $(MAJOR).$(MINOR)
RPM_VERSION = $(MAJOR).$(MINOR).0.$(COMMITVERSION)

