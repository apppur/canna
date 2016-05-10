CFLAGS = -g -Wall -fpermissive

HIREDIS_INC ?= ../3rd/hiredis-0.13.3
HIREDIS_LIB ?= ../3rd/hiredis-0.13.3/libhiredis.a

ZEROMQ_INC ?= ../3rd/zeromq/include
ZEROMQ_LIB ?= ../3rd/zeromq/lib/libzmq.a

MONGO_INC ?= ../3rd/libmongo/include/libmongoc-1.0
MONGO_LIB ?= ../3rd/libmongo/lib

BSON_INC ?= ../3rd/libmongo/include/libbson-1.0
BSON_INC ?= ../3rd/libmongo/lib

BODIUM_LIB ?= /usr/local/lib/libsodium.a

CORE_INC ?= ../core

INC := -I$(CORE_INC) -I$(HIREDIS_INC) -I$(ZEROMQ_INC) -I$(MONGO_INC) -I$(BSON_INC)
SRC := $(wildcard *.cpp)
LIBS := -lpthread -lmongoc-1.0 -lbson-1.0

CC = g++ --std=c++11

all : ventilator taskwork tasksink identitycheck routerreq routerdealer lbbroker asyncdeamon \
	peering

ventilator : ventilator.cpp ../core/canna_daemon.cpp ../core/canna_core.cpp
	$(CC) -o $@ $^ -I$(CORE_INC) -I$(ZEROMQ_INC) $(ZEROMQ_LIB) -lpthread

taskwork : taskwork.cpp 
	$(CC) -o $@ $^ -I$(ZEROMQ_INC) $(ZEROMQ_LIB) -lpthread

tasksink : tasksink.cpp 
	$(CC) -o $@ $^ -I$(ZEROMQ_INC) $(ZEROMQ_LIB) -lpthread

identitycheck : identitycheck.cpp ../core/canna_core.cpp
	$(CC) -o $@ $^ -I$(CORE_INC) -I$(ZEROMQ_INC) $(ZEROMQ_LIB) -lpthread

routerreq : routerreq.cpp ../core/canna_core.cpp
	$(CC) -o $@ $^ -I$(CORE_INC) -I$(ZEROMQ_INC) $(ZEROMQ_LIB) -lpthread

routerdealer : routerdealer.cpp ../core/canna_core.cpp
	$(CC) -o $@ $^ -I$(CORE_INC) -I$(ZEROMQ_INC) $(ZEROMQ_LIB) -lpthread

lbbroker : lbbroker.cpp ../core/canna_core.cpp
	$(CC) -o $@ $^ -I$(CORE_INC) -I$(ZEROMQ_INC) $(ZEROMQ_LIB) -lpthread

asyncdeamon : asyncdeamon.cpp ../core/canna_core.cpp
	$(CC) -o $@ $^ -I$(CORE_INC) -I$(ZEROMQ_INC) $(ZEROMQ_LIB) -lpthread

peering : peering.cpp ../core/canna_core.cpp
	$(CC) -o $@ $^ -I$(CORE_INC) -I$(ZEROMQ_INC) $(ZEROMQ_LIB) -lpthread

clean :
	rm ventilator taskwork tasksink identitycheck routerreq routerdealer lbbroker asyncdeamon \
		peering
