#include <stdio.h>
#include "redis_client.h"

redis_client::redis_client()
{
}

redis_client::~redis_client()
{
    if (m_context != nullptr) {
        freecontext(m_context);
    }
}

void redis_client::initialize(std::string host, int port)
{
    m_context = nullptr;
    m_host = host;
    m_port = port;
}

int redis_client::connect()
{
    m_context = redisConnect(m_host.c_str(), m_port);
    if (m_context == nullptr || m_context->err) {
        if (m_context) {
            fprintf(stderr, "redis-client connect failed, error: %s, code: %d\n", m_context->errstr, m_context->err);
            return -1;
        } else {
            fprintf(stderr, "redis-client allocate redis context failed\n");
            return -1;
        }
    } else {
        fprintf(stdout, "redis-client connect success, host: %s, port: %d\n", m_host.c_str(), m_port);
    }
    return 0;
}
