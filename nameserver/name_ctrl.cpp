#include "name_ctrl.h"
#include <stdio.h>
#include <string.h>

name_ctrl::name_ctrl()
{
}

name_ctrl::~name_ctrl()
{
}

std::string name_ctrl::allotnamepair(std::string name)
{
    std::unordered_map<std::string, name_pair>::iterator iter = m_serialmap.find(name);
    if (iter != m_serialmap.end()) {
        if (iter->second.bitset.all()) {
            return "";
        }
        int serial = iter->second.serial;
        std::bitset<SERIAL_SIZE> bitset;
        bitset = iter->second.bitset;
        for ( ; ; ) {
            serial = (serial+1) & 0xff;
            if (!bitset.test(serial)) {
                bitset.set(serial);
                iter->second.serial = serial;
                iter->second.bitset = bitset;
                char buff[128];
                memset(buff, 0, sizeof(buff));
                sprintf(buff, "%.4o", serial);
                printf("serail: %d, buff: %s\n", serial, buff);
                std::string str = buff;
                return (name+str);
            }
        }
    } else {
        name_pair namepair;
        namepair.serial = 0;
        std::pair<std::string, name_pair> newpair(name, namepair);
        m_serialmap.insert(newpair);
        return (name+"0");
    }
}
