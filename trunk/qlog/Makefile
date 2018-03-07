# set CPUS for Linux or FreeBSD
CPUS := $(shell cat /proc/cpuinfo|grep -c processor)

GFLAGS=gflags-2.0

GCC472_PATH=/usr/local/gcc-4.7.2
CXX := env LD_LIBRARY_PATH=$(GCC472_PATH)/lib:/usr/local/mpc/lib $(GCC472_PATH)/bin/g++
CXXFLAGS := -g3 -O2 -fno-strict-aliasing -Wall -Werror -fPIC \
	-I ../3rd_party/gflags/include \
	-I../ \
	-I../3rd_party/log4cplus/include/\
	-I../3rd_party/boost/include/\
	-isystem /usr/local/include \
	-isystem /usr/include 

LDFLAGS := -pthread \
	-L/usr/local/lib64/ \
	-L/usr/local/lib/ \
	-L../3rd_party/log4cplus/lib\
	-L../3rd_party/boost/lib/\
	-L../

RTFLAGS := \
	-Wl,-rpath=.. \
	-Wl,-rpath=../3rd_party/boost/lib/\
	-Wl,-rpath=../3rd_party/log4cplus/lib\
	-Wl,-rpath=/$(GCC472_PATH)/lib64

LIBS :=-llog4cplus -lz -lboost_thread\
	../3rd_party/gflags/lib/libgflags.a

SRC := $(wildcard *.cc) 
OBJ := $(patsubst %.cc, %.o, $(SRC))
DEP := $(patsubst %.o, %.d, $(OBJ))

TARGET := libqlog.so

libqlog.so: qlog.o
	$(CXX) $^ -o $@ $(RTFLAGS) $(LDFLAGS) $(LIBS) -shared

target: $(TARGET)

%.o : %.cc
	$(CXX) -c $(CXXFLAGS) $< -o $@

%.d : %.cc
	@$(CXX) -MM $< $(CXXFLAGS) | sed 's/$(notdir $*)\.o/$(subst /,\/,$*).o $(subst /,\/,$*).d/g' > $@

clean:
	-rm -rf $(OBJ) $(TARGET) ./package *.core $(DEP) *_unittest

test: all
	./acl_unittest
	./auth_unittest
install: all
	make clean && make -j 10
	mkdir -p ./package/qlog/include/qlog/ 
	mkdir -p ./package/qlog/lib 
	cp *.h ./package/qlog/include/qlog
	cp *.so ./package/qlog/lib
	cp version ./package/qlog/

.PHONY: all target clean test
