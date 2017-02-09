include ../build.mk/astyle.mk

CXX := g++

CURRENT_DIR=$(shell echo `pwd`)

GTESTDIR = ../3rdparty/gtest

CPPFLAGS= -g -c -fPIC \
		  -D_DEBUG -D_USE_GTEST \
		  -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings \
		  -Wsign-compare -Winvalid-pch -fms-extensions -Wall \
		  -MMD -Woverloaded-virtual -Wsign-promo -fno-gnu-keywords -std=c++11\
		  -I ./$(GTESTDIR) \
		  -I .. \
		  -I $(CURRENT_DIR)/../3rdparty \

RTFLAGS := \
    -Wl,-rpath=. \
    -Wl,-rpath=$(GCC472_PATH)/lib64 \
    -Wl,-rpath=$(CURRENT_DIR)/../evpp

LDFLAGS= $(RTFLAGS) \
	 -L $(CURRENT_DIR)/../evpp \
	 -lpthread \
	 -levpp -levent -lglog

SRCS := $(wildcard *.cc) $(GTESTDIR)/src/gtest-all.cc $(GTESTDIR)/src/gtest_main.cc
OBJS := $(patsubst %.cc, %.o, $(SRCS))
DEPS := $(patsubst %.o, %.d, $(OBJS))

TARGET=evpp-test

all : $(TARGET) 

check : $(TARGET)
	./$^

$(TARGET) : $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

-include $(DEPS)

%.o : %.cc
	$(CXX) $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -rf *.o *.d core.* $(OBJS) $(DEPS) $(TARGET) *.tmp decoded.txt log

fmt:
	$(ASTYLE) *.cc *.h
	rm -rf *.orig
