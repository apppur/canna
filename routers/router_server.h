#include <string>

#include "zmq.hpp"

class router_server
{
    public:
        router_server();
        ~router_server();
        int initialize();

    private:
        zmq::socket_t * sock_create(int type);
        void sock_bind(zmq::socket_t * sock, const char * addr);
        void sock_identity(zmq::socket_t * sock, const char * addr);

    private:
        zmq::context_t * m_context;
        zmq::socket_t * m_clusterfe;
        zmq::socket_t * m_clusterbe;

        zmq::socket_t * m_subscribe;
        zmq::socket_t * m_requester;
};
