#include "graph.h"

Graph::Graph(int sz) {
    m_size = sz;
    m_edges = new std::list<std::pair<int, int>> [m_size];
}

Graph::~Graph() {
    delete [] m_edges;
}

void Graph::AddEdge(int u, int v, int w) {
    m_edges[u].push_back(std::make_pair(v, w));
    m_edges[v].push_back(std::make_pair(u, w));
}


