#include <iostream>
#include "canna_core.h"
#include "router_server.h"
#include "nameinfo.pb.h"

int main(int argc, char ** argv)
{
    router_server routerserver;
    routerserver.initialize();
    routerserver.router_init();
    std::vector<std::string> namelist;
    namelist = routerserver.router_name();

    for (auto name : namelist) {
        std::cout << name << std::endl;
    }

    while (true) {
        canna_sleep(1000);
    }
}
