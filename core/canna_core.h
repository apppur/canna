#ifndef _CANNA_CORE_H
#define _CANNA_CORE_H
#include <stdint.h>
#include <string>

#include "zmq.hpp"

uint64_t canna_gettime();

std::string canna_recv(zmq::socket_t &socket);
bool canna_send(zmq::socket_t &socket, const std::string &string);
bool canna_sendmore(zmq::socket_t &socket, const std::string &string);
void canna_dump(zmq::socket_t &socket);

#endif
