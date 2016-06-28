PLATFORM := $(shell uname)
PLATFORM_VERSION := $(shell uname -r| awk -F 'el' '{printf("el%d", substr($$2,1,1))}')

CXX := g++
#ifeq ($(PLATFORM_VERSION),el6)
#GCC472_PATH=/home/s/gcc-4.9.2
#CXX := env LD_LIBRARY_PATH=$(GCC472_PATH)/lib $(GCC472_PATH)/bin/g++
#else
#CXX := g++
#endif

CURRENT_DIR=$(shell echo `pwd`)
PARENT_DIRNAME=$(shell basename $(shell dirname $(CURRENT_DIR)))
ROOT_DIRNAME=$(shell basename $(shell dirname $(PARENT_DIRNAME)))
SOURCE_DIR=$(CURRENT_DIR)
PREFIX=$(CURRENT_DIR)/../install

GTESTDIR = $(ROOT_DIRNAME)/3rdparty/gtest

CPPFLAGS= -g -c -fPIC \
		  -D_DEBUG -D_USE_GTEST \
		  -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings \
		  -Wsign-compare -Winvalid-pch -fms-extensions -Wall \
		  -MMD -Woverloaded-virtual -Wsign-promo -fno-gnu-keywords -std=c++11 \
		  -I ./$(GTESTDIR) \
		  -I ../include\
		  -I ../.. \
		  -I $(SOURCE_DIR)/../3rdparty \
		  -I /home/muyuwei/3rd/include

RTFLAGS := \
    -Wl,-rpath=. \
    -Wl,-rpath=$(GCC472_PATH)/lib64 \
    -Wl,-rpath=$(CURRENT_DIR)/../../src \
    -Wl,-rpath=/home/muyuwei/3rd/lib

LDFLAGS= $(RTFLAGS) \
		 -L $(CURRENT_DIR)/../src \
		 -L $(CURRENT_DIR)/../../src \
    	 -L /home/muyuwei/3rd/lib \
		 -lpthread \
		 -levpp -levent -lglog -lhashkit

DIRNAME=$(shell basename `pwd`)

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS))
DEPS := $(patsubst %.o, %.d, $(OBJS))
TARGET := evmc

all : $(TARGET) 

check : $(TARGET)
	./$^

$(TARGET) : $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

-include $(DEPS)

%.o : %.cc
	$(CXX) $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -rf *.o *.d $(OBJS) $(DEPS) $(TARGET) *.tmp decoded.txt log

