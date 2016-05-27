#ifndef _CANNA_CORE_H
#define _CANNA_CORE_H
#include <stdint.h>
#include <string>

#include "zmq.hpp"

#define CANNA_RAND(seed) (int) ((float)(seed)*random()/(RAND_MAX + 1.0))
#define DAEMON_MODE 1

uint64_t canna_gettime();

std::string canna_recv(zmq::socket_t &socket);
bool canna_send(zmq::socket_t &socket, const std::string &string);
bool canna_sendmore(zmq::socket_t &socket, const std::string &string);
void canna_dump(zmq::socket_t &socket);
std::string canna_setid(zmq::socket_t &socket);
void canna_sleep(int msecs);

#endif
