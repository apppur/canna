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
                memset(buff, '\0', sizeof(buff));
                sprintf(buff, "-%.4o", serial);
                printf("serail: %d, buff: %s\n", serial, buff);
                std::string str = buff;
                return (name+str);
            }
        }
    } else {
        name_pair namepair;
        namepair.serial = 0;
        namepair.bitset.set(0);
        std::pair<std::string, name_pair> newpair(name, namepair);
        m_serialmap.insert(newpair);
        printf("serail: 0, buff: %s-0000\n", name.c_str());
        return (name+"-0000");
    }
}

std::vector<std::string> name_ctrl::getnamelist(std::string name)
{
    std::vector<std::string> namelist;
    std::unordered_map<std::string, name_pair>::iterator iter = m_serialmap.find(name);
    if (iter != m_serialmap.end()) {
        std::bitset<SERIAL_SIZE> bitset = iter->second.bitset;
        int total = bitset.count();
        int count = 0;
        for (int i = 0; i < SERIAL_SIZE; i++) {
            if (bitset.test(i)) {
                count++;
                char buff[128];
                memset(buff, '\0', sizeof(buff));
                sprintf(buff, "-%.4o", i);
                std::string str = name + buff;
                namelist.push_back(str);
                if (count >= total) {
                    break;
                }
            }
        }
    }

    return namelist;
}

