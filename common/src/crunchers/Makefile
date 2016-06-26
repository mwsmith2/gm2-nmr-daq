# Grab the targets and sources as two batches
SOURCES = $(wildcard src/*.cxx)
OBJECTS = $(patsubst src/%.cxx,build/%.o,$(SOURCES))
TARGETS = $(patsubst src/%.cxx,bin/%,$(SOURCES))

# Debugging/profiling flags
# DEBUG = -g -pg

# Figure out the architecture
UNAME_S = $(shell uname -s)

# Clang compiler
ifeq ($(UNAME_S), Darwin)
	CXX = clang++
	CC  = clang
	FLAGS = -std=c++11 $(DEBUG)
endif

# Gnu compiler
ifeq ($(UNAME_S), Linux)
	CXX = g++
	CC  = gcc
	FLAGS = -std=c++0x $(DEBUG)
endif

FLAGS += -Iinclude -Icore/include -Icore
LIBS += -Wl,-rpath,/usr/local/lib -L/usr/lib64/mysql -L/usr/local/lib
LIBS += -lfftw3 -lm -lmysqlclient -lsqlite3 -lboost_filesystem -lboost_system -lfid
LIBS += -Lcore/lib -lgm2fieldcore -lodbc

# ROOT flags
FLAGS += $(shell root-config --cflags)
LIBS  += $(shell root-config --libs)

# MIDAS flags
FLAGS += -I$(MIDASSYS)/include
LIBS  += -L$(MIDASSYS)/linux/lib -lmidas-shared

all: $(TARGETS)

build/%.o: src/%.cxx
	$(CXX) $(FLAGS) -o $@ -c $<

bin/%: build/%.o
	$(CXX) $(FLAGS) -o $@ $+ $(LIBS)

clean:
	rm -f $(OBJECTS) $(TARGETS)
