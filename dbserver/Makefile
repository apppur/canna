CFLAGS = -g -O2 -Wall -fpermissive

HIREDIS_INC ?= ../3rd/hiredis-0.13.3
HIREDIS_LIB ?= ../3rd/hiredis-0.13.3/libhiredis.a

ZEROMQ_INC ?= ../3rd/zeromq/include
ZEROMQ_LIB ?= ../3rd/zeromq/lib/libzmq.a

BODIUM_LIB ?= /usr/local/lib/libsodium.a

CC = g++ --std=c++11

all : dbserver client

dbserver : main.cpp ../core/canna_daemon.cpp
	$(CC) -o $@ $^ -I$(HIREDIS_INC) -I$(ZEROMQ_INC) $(HIREDIS_LIB) $(ZEROMQ_LIB) $(BODIUM_LIB) -lpthread

client : test.cpp
	$(CC) -o $@ $^ -I$(ZEROMQ_INC) $(ZEROMQ_LIB) $(BODIUM_LIB) -lpthread

clean :
	rm dbserver client
