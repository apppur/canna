#ifndef _REDIS_CLIENT_H
#define _REDIS_CLIENT_H

#include <string>
#include "hiredis.h"

class redis_client
{
    public:
        redis_client();
        ~redis_client();
        void initialize(std::string host, int port);

    public:
        int connect();

        std::string gethost() { return m_host; }
        void sethost(std::string host) { m_host = host; }

        int getport() { return m_port; }
        void setport(int port) { m_port = port; }

        void freecontext(redisContext * context) { redisFree(context); }
    private:
        redisContext * m_context;
        std::string m_host;
        int m_port;
};

#endif
