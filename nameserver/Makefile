CFLAGS = -g -O2 -Wall 

HIREDIS_INC ?= ../3rd/hiredis-0.13.3
HIREDIS_LIB ?= ../3rd/hiredis-0.13.3/libhiredis.a

ZEROMQ_INC ?= ../3rd/zeromq/include
ZEROMQ_LIB ?= ../3rd/zeromq/lib/libzmq.a

PROTOBUF_INC ?= ../3rd/libprotobuf/include
PROTOBUF_LIB ?= ../3rd/libprotobuf/lib/libprotobuf.a

PROTOFILE_INC ?= ../protos

CORE_INC ?= ../core

INC := -I$(CORE_INC) -I$(HIREDIS_INC) -I$(ZEROMQ_INC) -I$(PROTOBUF_INC) -I$(PROTOFILE_INC)
SRC := $(wildcard *.cpp)
LIBS := -lpthread

CC = g++ --std=c++11

all : nameserver client

nameserver : main.cpp redis_client.cpp name_ctrl.cpp name_server.cpp ../protos/nameinfo.pb.cc ../core/canna_daemon.cpp ../core/canna_core.cpp
	$(CC) -o $@ $^ $(INC) $(LIBS) $(HIREDIS_LIB) $(ZEROMQ_LIB) $(PROTOBUF_LIB)

client : test.cpp ../protos/nameinfo.pb.cc ../core/canna_core.cpp
	$(CC) -o $@ $^ $(INC) $(LIBS) $(ZEROMQ_LIB) $(PROTOBUF_LIB)

clean :
	rm nameserver client
