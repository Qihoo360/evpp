
CC=gcc
CXX=g++
AR=ar
ARFLAGS=cru
CURRENT_DIR=$(shell echo `pwd`)
SOURCE_DIR=$(CURRENT_DIR)
CFLAGS=$(TOP_CFLAGS) -g -c -fPIC \
	   -Wall -Wno-unused-parameter -Wno-unused-function -Wunused-variable -Wunused-value \
	   -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings -Wsign-compare \
	   -Winvalid-pch -fms-extensions -Wfloat-equal -Wextra \
	   -std=c++11 \
	   -I ../../include -MMD
CPPFLAGS= $(CFLAGS) -Woverloaded-virtual -Wsign-promo -fno-gnu-keywords
LDFLAGS=$(TOP_LDFLAGS) -lpthread -levent -lglog

LIBNAME=evppbase

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS)) $(CCOBJS)
DEPS := $(patsubst %.o, %.d, $(OBJS))

TARGET_SO=lib$(LIBNAME).so
TARGET_A=lib$(LIBNAME).a

all : $(TARGET_SO) $(TARGET_A)
	$(MAKE) install

$(TARGET_A) : $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) 
	ranlib $@			   

$(TARGET_SO) : $(OBJS)
	$(CXX) $(LDFLAGS)  $(OBJS) -shared -o $@
	
check : $(TARGET_A) $(TARGET_SO)	
	$(MAKE) install
	$(MAKE) check -C test
	$(MAKE) check -C quick_test
	$(MAKE) check -C quick_gtest

%.o : %.cc 
	$(CXX) $(CPPFLAGS) $< -o $@

-include $(DEPS)

t:
	@echo "LIBNAME:" $(LIBNAME)
	@echo "SRCS:" $(SRCS)
	@echo "OBJS:" $(OBJS)
	@echo "CFLAGS:" $(CFLAGS)
	@echo "CPPFLAGS:" $(CPPFLAGS)

install: $(TARGET_A) $(TARGET_SO) 

clean:
	rm -rf src/*.o src/*.d $(OBJS) $(DEPS) $(TARGET_SO) $(TARGET_A)

