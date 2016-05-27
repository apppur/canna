#ifndef _NAME_CTRL_H
#define _NAME_CTRL_H

#include <string>
#include <unordered_map>
#include <bitset>

#define SERIAL_SIZE 256
struct name_pair
{
    int serial;
    std::bitset<SERIAL_SIZE> bitset;
};

class name_ctrl
{
    public:
        name_ctrl();
        ~name_ctrl();

        //std::vector<name_pair> getnamelist(std::string name);
        std::string allotnamepair(std::string name);
        //bool existname(std::string name);

    private:
        std::unordered_map<std::string, name_pair> m_serialmap;
        //std::unordered_map<std::string, vector<int>> m_namepool;
};

#endif
