#include <string>
#include <vector>

#include "zmq.hpp"

class router_server
{
    public:
        router_server();
        ~router_server();
        int initialize();

    public:
        void router_init();
        std::vector<std::string> router_name();

    private:
        zmq::socket_t * sock_create(int type);
        void sock_bind(zmq::socket_t * sock, const char * addr);
        void sock_identity(zmq::socket_t * sock, const char * addr);
        void sock_setfilter(zmq::socket_t * sock, const char * filter);
        void sock_connect(zmq::socket_t * sock, const char * addr);

        std::string sock_recv(zmq::socket_t &socket);
        bool sock_send(zmq::socket_t &socket, const std::string &string);
        bool sock_sendmore(zmq::socket_t &socket, const std::string &string);

    private:
        zmq::context_t * m_context;
        zmq::socket_t * m_clusterfe;
        zmq::socket_t * m_clusterbe;

        zmq::socket_t * m_subscribe;
        zmq::socket_t * m_requester;
};
