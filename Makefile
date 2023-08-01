# Makefile.

# Copyright (C) 2015-2015 YOYOSYS, Inc.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

#
# Usage: 
#   make
# But on ubuntu
#   make OS=ubuntu
#

BIN_DIR=../bin
BITSFLOW_HOME ?= /app/wfw/env/bitsflow
BITSFLOW_HL_HOME ?= /app/wfw/env/bitsflow
BITSFLOW_3RD_EX_HOME ?= /app/wfw/env/bitsflow/3rdparty_extends

CC = g++

CPPFLAGS := -O0 -pthread
CPPFLAGS += -DBITSFLOW_MAJOR=2 -DBITSFLOW_MINOR=27 -DBITSFLOW_MAINTENANCE=34 -DBITSFLOW_APPEND=0
CPPFLAGS += -g -ggdb3 -DDEBUG
CPPFLAGS += -Wall -W -Wno-deprecated
CPPFLAGS += -D__LINUX__
CPPFLAGS += -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
ifeq ($(OS), ubuntu)
CPPFLAGS += -D__LINUX_UBUNTU_XENIAL__ -D_GLIBCXX_USE_CXX11_ABI=0
endif
CPPFLAGS += -D__LINUX_NEOKYLIN_5__ 
CPPFLAGS += -D_GLIBCXX_USE_CXX11_ABI=1 
CPPFLAGS += -I$(BITSFLOW_HOME)/include
CPPFLAGS += -I$(BITSFLOW_HL_HOME)/include
CPPFLAGS += -I$(BITSFLOW_3RD_EX_HOME)/include

CXXFLAGS := -fno-stack-protector -fPIC

LDFLAGS  := -pthread -Wl,-Bsymbolic
LDFLAGS  += -L${BITSFLOW_HOME}/lib
LDFLAGS  += -L${BITSFLOW_HL_HOME}/output/linux.rhel.6.amd64.release/lib
LDFLAGS  += -L${BITSFLOW_3RD_EX_HOME}/lib
LDFLAGS  += -Wl,-rpath-link,${BITSFLOW_HOME}/lib
LDFLAGS  += -lbitsflow++ -lyoyocore++ -lbitsflow -lyoyocore_regex -lyoyocore_storage -lyoyocore_event -lyoyocore -lyoyodefs -lbitsflow-hl++ -lavro

.SUFFIXES: .cc .o

%.d: %.cc
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.MAKE: all

HEADERS = $(wildcard *.h)
SOURCES = $(wildcard *.cpp)
TARGETS = $(SOURCES:.cpp=)

all: ${BIN_DIR}/PublishSimple ${BIN_DIR}/SubscribeSimple

${BIN_DIR}/PublishSimple: PublishSimple.o
	$(CC) $^ -o $@ $(LDFLAGS)

${BIN_DIR}/SubscribeSimple: SubscribeSimple.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -fr *.o *.d *.d.* ${BIN_DIR}/PublishSimple ${BIN_DIR}/SubscribeSimple

makebin:
	$(shell if [ ! -d ${BIN_DIR} ]; then mkdir -pv ${BIN_DIR}; fi)

-include $(SOURCES:.cpp=.d)

.PHONY: all clean makebin

.NOEXPORT:
