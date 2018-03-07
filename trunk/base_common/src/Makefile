include ../version

PLATFORM_VERSION := $(shell uname -r| awk -F 'el' '{printf("%d", substr($$2,1,1))}')

LIBNAME = lib$(MODULE_NAME).so
SONAME_VERSION = $(shell echo $(MODULE_VERSION) | cut -d. -f1)
PACKAGE_NAME=$(MODULE_NAME)-$(MODULE_VERSION)

GCC472_PATH=/usr/local/gcc-4.7.2
CXX11 := env LD_LIBRARY_PATH=$(GCC472_PATH)/lib:/usr/local/mpc/lib $(GCC472_PATH)/bin/g++
CXXFLAGS := -g -fno-strict-aliasing -Wall -std=c++11 -D__STDC_FORMAT_MACROS -Wno-deprecated -Wno-sign-compare -fPIC \
	-I../../3rd_party/boost-${boost_VERSION}/include\
	-I../../3rd_party/qlog/include/

LDFLAGS := -pthread \
	-L../../3rd_party/boost-${boost_VERSION}/lib \
	-L../../3rd_party/qlog/lib\
	-L/usr/lib64 \
	-L/usr/local/lib64/ 

RTFLAGS := -Wl,-rpath=../../3rd_party/qlog/lib\
		-Wl,-rpath=../../3rd_party/boost-${boost_VERSION}/lib/

LIBS := -lssl \
		-lqlog\
		-levent 
SRC := $(wildcard *.cc) \
	   $(wildcard ../*.cc)
OBJ := $(patsubst %.cc, %.o, $(SRC))
DEP := $(patsubst %.o, %.d, $(OBJ))

MAJOR=$(shell echo $(MODULE_VERSION) | cut -d. -f1)

all: 
	$(MAKE) $(LIBNAME)

$(LIBNAME): $(OBJ)
	$(CXX11) $^ -Wl,-soname,$(@).$(MAJOR) -o $@.$(MODULE_VERSION) $(LDFLAGS) $(RTFLAGS) $(LIBS) -shared
	rm -fr $(@)
	rm -fr $(@).$(MAJOR)
	/sbin/ldconfig -n .
	ln -s $(@).$(MAJOR) $(@)

%.o : %.cc
	$(CXX11) -c $(CXXFLAGS) $< -o $@
%.o : %.cpp
	$(CXX11) -c $(CXXFLAGS) $< -o $@

%.d : %.cc
	@$(CXX11) -MM $< $(CXXFLAGS) | sed 's/$(notdir $*)\.o/$(subst /,\/,$*).o $(subst /,\/,$*).d/g' > $@
%.d : %.cpp
	@$(CXX11) -MM $< $(CXXFLAGS) | sed 's/$(notdir $*)\.o/$(subst /,\/,$*).o $(subst /,\/,$*).d/g' > $@

clean:
	-rm -rf $(OBJ) *.pid *.log *.core $(DEP) $(GEN_DEP) *.so.* *.d *.so

install:
	/usr/bin/install -c $(LIBNAME).$(MODULE_VERSION) ../packages/$(PACKAGE_NAME)/lib/
	/sbin/ldconfig -n ../packages/$(PACKAGE_NAME)/lib
	(cd ../packages/$(PACKAGE_NAME)/lib; ln -s $(LIBNAME).$(SONAME_VERSION) $(LIBNAME))

.PHONY: clean install
