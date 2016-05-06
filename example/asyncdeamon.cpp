#include <thread>
#include <functional>
#include <stdio.h>
#include <memory>
#include "asyncclient.h"
#include "asyncserver.h"

int main(int argc, char** argv)
{
    asyncclient async1;
    asyncclient async2;
    asyncclient async3;

    asyncserver server;

    std::thread t1(std::bind(&asyncclient::start, &async1));
    std::thread t2(std::bind(&asyncclient::start, &async2));
    std::thread t3(std::bind(&asyncclient::start, &async3));
    std::thread t4(std::bind(&asyncserver::run, &server));

    t1.detach();
    t2.detach();
    t3.detach();
    t4.detach();

    getchar();

    return 0;
}
