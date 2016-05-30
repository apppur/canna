CFLAGS = -g -O2 -Wall -fpermissive

PROTOBUF_INC ?= ../3rd/libprotobuf/include
PROTOBUF_LIB ?= ../3rd/libprotobuf/lib/libprotobuf.a

ZEROMQ_INC ?= ../3rd/zeromq/include
ZEROMQ_LIB ?= ../3rd/zeromq/lib/libzmq.a

CORE_INC ?= ../core

INC := -I$(CORE_INC) -I$(PROTOBUF_INC)
SRC := $(wildcard *.cpp)
LIBS := -lpthread -lprotobuf

CC = g++ --std=c++11

all : test_proto

test_proto : test_proto.cpp addressbook.pb.cc
	$(CC) -o $@ $^ $(INC) $(PROTOBUF_LIB)

clean :
	rm test_proto
