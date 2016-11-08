#####################################################################
#
#  Name:         Makefile
#  Created by:   Matthias Smith
#
#  Contents:     Makefile for MIDAS NMR frontends, based on
#		 a sample Makefile written by Stefan Ritt.
#
#####################################################################

NMR_DIR = core
OBJECTS = $(patsubst obj/%.cxx, build/%.o, $(wildcard obj/*.cxx))
TARGETS = $(patsubst src/%.cxx,bin/%,$(wildcard src/*.cxx))

#-------------------------------------------------------------------
# This is for Linux
OSFLAGS = -DOS_LINUX -DHAVE_USB -DHAVE_LIBUSB10 -Dextname
CFLAGS = -fpermissive -O2
CXXFLAGS = -std=c++0x
LIBS = -lm -lz -lpthread -lfftw3 -lzmq -lodbc -lrt -lyocto -lyapi

ROOTFLAGS = $(shell root-config --cflags)
ROOTLIBS = $(shell root-config --libs)

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
INC_DIR = $(MIDASSYS)/include
LIB_DIR = $(MIDASSYS)/linux/lib
LINK_DIR = ../../../common/

# Libraries && include flags
LIB = -L$(LIB_DIR) -L$(NMR_DIR)/lib -L/usr/lib64/mysql
LIB += -lmidas-shared -lmysqlclient -lsqlite3 -lCAENDigitizer -lusb-1.0
LIB += -ljson_spirit -lfid -lgm2fieldcore -llabdaq -lcurl
LIB += -lboost_system -lboost_filesystem
CFLAGS += -I$(INC_DIR) -I$(NMR_DIR)/include -Iobj -Iinclude
CFLAGS += -I/usr/local/include/labdaq
CFLAGS += -Wl,-rpath,/usr/local/lib

# Set compilers
CC = gcc
CXX = g++

ifdef DEBUG
	CC += -pg -g
	CXX += -pg -g
else
	CC += -O3
	CXX += -O3
endif

#-------------------------------------------------------------------
# Finally we have the actual make directives.

# Make commands

all: $(OBJECTS) $(TARGETS)

build/%.o: obj/%.cxx obj/%.hh
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(OSFLAGS) $(ROOTFLAGS) -c $< -o $@

bin/%: src/%.cxx $(LIB_DIR)/mfe.o $(OBJECTS)
	$(CXX) -o $@ $+ $(CXXFLAGS) $(CFLAGS) $(OSFLAGS) \
	$(LIB) $(LIBS) $(ROOTFLAGS) $(ROOTLIBS)
	ln -fs $(shell pwd)/$@ $(LINK_DIR)/$@

clean:
	rm -f *~ $(TARGETS) $(OBJECTS)
