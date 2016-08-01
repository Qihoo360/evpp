PLATFORM := $(shell uname)
PLATFORM_VERSION := $(shell uname -r| awk -F 'el' '{printf("el%d", substr($$2,1,1))}')

CXX := g++

CURRENT_DIR=$(shell pwd)
PROJECT_ROOT_DIR=$(CURRENT_DIR)/../..
THIRD_PARTY_DIR=$(PROJECT_ROOT_DIR)/3rdparty

CPPFLAGS= -g -c -fPIC \
		  -O3 \
		  -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings \
		  -Wsign-compare -Winvalid-pch -fms-extensions -Wall \
		  -MMD -Woverloaded-virtual -Wsign-promo -fno-gnu-keywords -std=c++11 -DGOOGLE_STRIP_LOG=1 \
		  -I . \
		  -I $(PROJECT_ROOT_DIR) \
		  -I $(THIRD_PARTY_DIR)

RTFLAGS := \
		   -Wl,-rpath=. \
		   -Wl,-rpath=$(PROJECT_ROOT_DIR)/evpp \
		   -Wl,-rpath=$(THIRD_PARTY_DIR)

LDFLAGS= $(RTFLAGS) \
		 -L$(PROJECT_ROOT_DIR)/evpp \
		 -L$(THIRD_PARTY_DIR) \
		 -lpthread \
		 -levpp -levent -lglog -lhashkit

DIRNAME=$(shell basename `pwd`)

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS))
DEPS := $(patsubst %.o, %.d, $(OBJS))

TARGET := libevmc.a

all : dependency $(TARGET) test

check : $(TARGET)
	./$^

test :
	$(MAKE) -C test

dependency :
	$(MAKE) -C $(THIRD_PARTY_DIR)/libhashkit
	$(MAKE) -C $(PROJECT_ROOT_DIR)/evpp

$(TARGET) : $(OBJS)
	# $(CXX) $(OBJS) $(LDFLAGS) -o $@
	ar cru $@ $(OBJS)
	ranlib $@

-include $(DEPS)

%.o : %.cc
	$(CXX) $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -rf *.o *.d $(OBJS) $(DEPS) $(TARGET) *.tmp decoded.txt log
	$(MAKE) clean -C test

.PHONY : test dependency
