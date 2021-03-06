#include "graph.h"
#include "dijkstra.h"
#include "thread_safe_queue.h"
#include "thread_pool.h"

#include <unistd.h>
#include <iostream>
#include <functional>

void Sum() {
    int sum = 0;
    for (int i = 0; i < 100000000; i++) {
        sum += i;
    }
    std::cout << "Hello Concurrency! Sum:" << sum << std::endl;
}

int main() {
    int v = 9;
    Graph graph(v);

    graph.AddEdge(0, 1, 4);
    graph.AddEdge(0, 7, 8);
    graph.AddEdge(1, 2, 8);
    graph.AddEdge(1, 7, 11);
    graph.AddEdge(2, 3, 7);
    graph.AddEdge(2, 8, 2);
    graph.AddEdge(2, 5, 4);
    graph.AddEdge(3, 4, 9);
    graph.AddEdge(3, 5, 14);
    graph.AddEdge(4, 5, 10);
    graph.AddEdge(5, 6, 2);
    graph.AddEdge(6, 7, 1);
    graph.AddEdge(6, 8, 6);
    graph.AddEdge(7, 8, 7);

    Dijkstra dijkstra;
    dijkstra.ShortPath(graph, 0);
    dijkstra.GetPath(graph, 0, 4);
    dijkstra.GetPath(graph, 0, 8);
    dijkstra.GetPath(graph, 7, 2);

    ThreadPool pool;
    std::function<void()> fun = []() {std::cout << "Hello Concurrency!" << std::endl;};
    pool.submit(fun);

    for (int i = 0; i < 20; i++) {
        std::function<void()> fun = Sum;
        pool.submit(fun);
    }

    while (true) {
        sleep(3);
    }

    return 0;
}
