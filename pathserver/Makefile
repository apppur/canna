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

all : pathserver 

pathserver: main.cpp graph.cpp dijkstra.cpp 
	$(CC) -o $@ $^

clean :
	rm pathserver
