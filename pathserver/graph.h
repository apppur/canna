#ifndef _GRAPH_H
#define _GRAPH_H

#include <list>

typedef std::pair<int, int> pair_t;

class Graph {
    public:
        Graph(int size);
        ~Graph();

        void AddEdge(int u, int v, int w);

    private:
        int m_size;
        std::list<std::pair<int, int>> * m_edges;

        friend class Dijkstra;
};

#endif
