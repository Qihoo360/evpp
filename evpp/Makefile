include ../rpm/build.mk
include ../build.mk/astyle.mk

CC=gcc
CXX=g++
AR=ar
ARFLAGS=cru
CURRENT_DIR=$(shell echo `pwd`)
SOURCE_DIR=$(CURRENT_DIR)


DEBUG := -D_DEBUG

CFLAGS=$(TOP_CFLAGS) -g -c -fPIC $(DEBUG) \
	   -Wall -Wno-unused-parameter -Wno-unused-function -Wunused-variable -Wunused-value \
	   -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings -Wsign-compare \
	   -Winvalid-pch -fms-extensions -Wfloat-equal -Wextra -std=c++11 \
	   -I .. -I ../include -MMD
CPPFLAGS= $(CFLAGS) -Woverloaded-virtual -Wsign-promo -fno-gnu-keywords

RTFLAGS := \
    -Wl,-rpath=.

LDFLAGS= $(RTFLAGS) -lpthread \
		 -levent -lglog


SRCS := $(wildcard *.cc) $(wildcard httpc/*.cc) $(wildcard http/*.cc) $(wildcard udp/*.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS)) $(CCOBJS)
DEPS := $(patsubst %.o, %.d, $(OBJS))

LIBNAME=evpp
TARGET_SO=lib$(LIBNAME).so
TARGET_A=lib$(LIBNAME).a

all : $(TARGET_SO) $(TARGET_A)
	$(MAKE) install

$(TARGET_A) : $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) 
	ranlib $@			   

$(TARGET_SO) : $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -shared -o $@.$(FULL)
	@rm -fr $(@)
	@rm -fr $(@).$(MAJOR)
	ln -s $(@).$(FULL) $(@).$(MAJOR)
	ln -s $(@).$(MAJOR) $(@)
	
check : $(TARGET_A) $(TARGET_SO)	
	$(MAKE) install
	$(MAKE) check -C test
	$(MAKE) check -C quick_test
	$(MAKE) check -C quick_gtest

%.o : %.cc 
	$(CXX) $(CPPFLAGS) $< -o $@

-include $(DEPS)

install: $(TARGET_A) $(TARGET_SO) 

clean:
	rm -rf $(OBJS) $(DEPS) $(TARGET_SO) $(TARGET_A)

fmt:
	$(ASTYLE) *.cc *.h
	rm -rf *.orig
