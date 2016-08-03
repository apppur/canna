#ifndef _DIJKSTRA_H
#define _DIJKSTRA_H
#include "graph.h"

const int INF = 2016062613;

class Dijkstra {
    public:
        Dijkstra() {}
        ~Dijkstra() {}

        void ShortPath(const Graph & graph, int src);
        void GetPath(const Graph & graph, int src, int des);
};

#endif
