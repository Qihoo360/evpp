PROJ  = evmc
DEPEND_PROJ = 3rdparty

SRC			= $(PROJ).tar.gz
DEPEND_SRC	= $(DEPEND_PROJ).tar.gz
PKG 		= $(wildcard *.spec)
TEMP_DIR 	= /home/$(shell whoami)/tmp
ROOT_DIR 	= $(shell PWD=$$(pwd); echo $${PWD%%/$(PROJ)*}/$(PROJ))
THIRD_PARTY_DIR = $(ROOT_DIR)/../../3rdparty
HASHKIT_DIR = $(ROOT_DIR)/../../3rdparty/libhashkit
MEMCACHE_DIR = $(ROOT_DIR)/../../3rdparty/memcached
JSON_DIR = $(ROOT_DIR)/../../3rdparty/rapidjson
COMMITVERSION  = $(shell git log --format=%H -n1 | cut -b 1-8 )
MAJOR := 1
MINOR := 0
FULL := $(MAJOR).$(MINOR)
RPM_VERSION = $(MAJOR).$(MINOR).0.$(COMMITVERSION)

