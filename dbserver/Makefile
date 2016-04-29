CFLAGS = -g -O2 -Wall -fpermissive

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

all : dbserver client

dbserver : main.cpp ../core/canna_daemon.cpp ../core/canna_core.cpp
	$(CC) -o $@ $^ $(INC) -L$(MONGO_LIB) $(LIBS) $(HIREDIS_LIB) $(ZEROMQ_LIB) $(BODIUM_LIB)

client : test.cpp
	$(CC) -o $@ $^ -I$(ZEROMQ_INC) $(ZEROMQ_LIB) $(BODIUM_LIB) -lpthread

clean :
	rm dbserver client
