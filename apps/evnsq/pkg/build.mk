PROJ  = evnsq

SRC			= $(PROJ).tar.gz
DEPEND_SRC	= $(DEPEND_PROJ).tar.gz
PKG 		= $(wildcard *.spec)
TEMP_DIR 	= /home/$(shell whoami)/tmp
ROOT_DIR 	= $(shell PWD=$$(pwd); echo $${PWD%%/$(PROJ)*}/$(PROJ))
THIRD_PARTY_DIR = $(ROOT_DIR)/../../3rdparty
HASHKIT_DIR = $(ROOT_DIR)/../../3rdparty/libhashkit
MEMCACHE_DIR = $(ROOT_DIR)/../../3rdparty/memcached
JSON_DIR = $(ROOT_DIR)/../../3rdparty/rapidjson
COMMITVERSION  = $(shell git rev-list --all|wc -l)
MAJOR := 1
MINOR := 0
FULL := $(MAJOR).$(MINOR)
RPM_VERSION = $(MAJOR).$(MINOR).0.$(COMMITVERSION)

