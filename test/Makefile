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
SOURCE_DIR=$(CURRENT_DIR)
PREFIX=$(CURRENT_DIR)/../install

GTESTDIR = ../3rdparty/gtest

CPPFLAGS= -g -c -fPIC \
		  -D_DEBUG -D_USE_GTEST \
		  -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings \
		  -Wsign-compare -Winvalid-pch -fms-extensions -Wall \
		  -MMD -Woverloaded-virtual -Wsign-promo -fno-gnu-keywords -std=c++11\
		  -I ./$(GTESTDIR) \
		  -I ../include\
		  -I $(SOURCE_DIR)/../3rdparty \

RTFLAGS := \
    -Wl,-rpath=. \
    -Wl,-rpath=$(GCC472_PATH)/lib64 \
    -Wl,-rpath=$(CURRENT_DIR)/../src/base \
    -Wl,-rpath=$(CURRENT_DIR)/../src

LDFLAGS= $(RTFLAGS) \
		 -L $(CURRENT_DIR)/../src/base \
		 -L $(CURRENT_DIR)/../src \
		 -lpthread \
		 -levpp -levppbase -levent -lglog \
		 -lboost_system-mt -lboost_filesystem-mt -lboost_thread-mt 

DIRNAME=$(shell basename `pwd`)

SRCS := $(wildcard *.cc) $(GTESTDIR)/src/gtest-all.cc $(GTESTDIR)/src/gtest_main.cc
OBJS := $(patsubst %.cc, %.o, $(SRCS))
DEPS := $(patsubst %.o, %.d, $(OBJS))

TARGET=$(PARENT_DIRNAME)

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

