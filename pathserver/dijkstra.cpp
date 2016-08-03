#include <stdio.h>
#include <queue>
#include "vector"
#include "dijkstra.h"

void Dijkstra::ShortPath(const Graph & graph, int src) {
    std::priority_queue<pair_t, std::vector<pair_t>, std::greater<pair_t>> prio_queue;
    std::vector<int> dist(graph.m_size, INF);
    prio_queue.push(std::make_pair(0, src));
    dist[src] = 0;

    while (!prio_queue.empty()) {
        int u = prio_queue.top().second;
        prio_queue.pop();

        std::list<std::pair<int, int>>::iterator iter;
        for (iter = graph.m_edges[u].begin(); iter != graph.m_edges[u].end(); iter++) {
            int v = (*iter).first;
            int w = (*iter).second;
            if (dist[v] > dist[u] + w) {
                dist[v] = dist[u] + w;
                prio_queue.push(std::make_pair(dist[v], v));
            }
        }
    }

    printf("Vertex Distance from Source\n");
    for (int i = 0; i < dist.size(); i++) {
        printf("%d \t\t %d\n", i, dist[i]);
    }
}
