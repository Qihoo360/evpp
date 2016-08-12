PLATFORM := $(shell uname)
PLATFORM_VERSION := $(shell uname -r| awk -F 'el' '{printf("el%d", substr($$2,1,1))}')

CXX := g++
#ifeq ($(PLATFORM_VERSION),el6)
#GCC472_PATH=/home/s/gcc-4.9.2
#CXX := env LD_LIBRARY_PATH=$(GCC472_PATH)/lib $(GCC472_PATH)/bin/g++
#else
#CXX := g++
#endif

CURRENT_DIR= $(shell pwd)
ROOT_DIR=$(shell cd ./../../.. && pwd)
PREFIX=$(CURRENT_DIR)/../install

CPPFLAGS= -c -fPIC -O3 \
		  -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings \
		  -Wsign-compare -Winvalid-pch -fms-extensions -Wall \
		  -MMD -Woverloaded-virtual -Wsign-promo -fno-gnu-keywords -std=c++11 -DGOOGLE_STRIP_LOG=1  -I ../include \
		  -I .. \
		  -I ../.. \
		  -I $(ROOT_DIR) \
		  -I $(ROOT_DIR)/3rdparty

RTFLAGS := \
    -Wl,-rpath=. \
    -Wl,-rpath=$(GCC472_PATH)/lib64 \
    -Wl,-rpath=$(ROOT_DIR)/evpp

LDFLAGS= $(RTFLAGS) \
		 -L $(ROOT_DIR)/evpp \
		 -L $(ROOT_DIR)/3rdparty/libhashkit \
		 -L .. \
		 -lpthread -levmc \
		 -levpp -levent  -lglog -lgflags -lhashkit

DIRNAME=$(shell basename `pwd`)

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS))
DEPS := $(patsubst %.o, %.d, $(OBJS))
TARGET := test_evmc

all : $(TARGET) test

check : $(TARGET)
	./$^

$(TARGET) : $(OBJS) ../libevmc.a $(ROOT_DIR)/evpp/libevpp.a
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

-include $(DEPS)

%.o : %.cc
	$(CXX) $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -rf *.o *.d $(OBJS) $(DEPS) $(TARGET) *.tmp

test_var :
	echo ${CURRENT_DIR}
	echo ${ROOT_DIR}

.PHONY : test test_var
