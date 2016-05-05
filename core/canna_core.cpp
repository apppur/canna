#include <sys/time.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include "canna_core.h"

uint64_t canna_gettime()
{
    uint64_t t = 0;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    t = (uint64_t)tv.tv_sec * 1000;
    t += tv.tv_usec / 1000;

    return t;
}

std::string canna_recv(zmq::socket_t &socket)
{
    zmq::message_t message;
    socket.recv(&message);

    return std::string(static_cast<char*>(message.data()), message.size());
}

bool canna_send(zmq::socket_t &socket, const std::string &string)
{
    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool result = socket.send(message);

    return result;
}

bool canna_sendmore(zmq::socket_t &socket, const std::string &string)
{
    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool result = socket.send(message, ZMQ_SNDMORE);

    return result;
}

void canna_dump(zmq::socket_t &socket)
{
    std::cout << "--------------------------------------------------" << std::endl;
    while (true) {
        zmq::message_t message;
        socket.recv(&message);

        int size = message.size();
        std::string data(static_cast<char *>(message.data()), size);

        bool is_text = true;
        unsigned int count;
        unsigned char c;
        for (count = 0; count < size; count++) {
            c = data[count];
            if (c < 32 || c > 127) {
                is_text = false;
            }
        }
        std::cout << "[" << std::setfill('0') << std::setw(3) << size << "]";
        for (count = 0; count < size; count++) {
            if (is_text) {
                std::cout << (char)data[count];
            } else {
                std::cout << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)data[count];
            }
        }
        std::cout << std::endl;

        int more = 0;
        size_t more_size = sizeof(more);
        socket.getsockopt (ZMQ_RCVMORE, &more, &more_size);
        
        if (!more) {
            break;
        }
    }
}
