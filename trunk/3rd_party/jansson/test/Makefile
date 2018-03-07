#Macros
INCLUDE = -I../Lib/3rdParty/jansson/include/ \

LIBPATH = ../Lib/3rdParty/jansson/lib/libjansson.a \

TESTLLDLIB = -lcrypto -lpthread -lm -lrt -lssl -lidn


ifeq ($(VERSION), debug)
    DEBUG_DEF = -g -D_DEBUG
else
    ifeq ($(VERSION), release)
        DEBUG_DEF = -O2
    else
        DEBUG_DEF = -g -O2
    endif
endif

DEBUG_DEF = -g -O2 -DVALGRIND_DEBUG

CPPFLAGS = -Wall -fPIC $(DEBUG_DEF) $(INCLUDE)
APP = test

#Actions
all : clean $(APP)

clean : 
	rm -r -f *.o *.so.* $(LIB)  test

test : test.o
	g++ $(CPPFLAGS) -o $@ $^ $(LIBPATH) $(TESTLLDLIB) 

.o : .cpp
	g++ ${CPPFLAGS} -c $^
