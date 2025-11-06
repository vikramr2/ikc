#include <iostream>
#include <string>
#include <thread>

#include "../lib/io/graph_io.h"
#include "../lib/data_structures/graph.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <graph_file.tsv> [num_threads]" << std::endl;
        return 1;
    }

    std::string graph_file = argv[1];
    int num_threads = std::thread::hardware_concurrency();

    if (argc >= 3) {
        num_threads = std::stoi(argv[2]);
    }

    std::cout << "Loading graph from: " << graph_file << std::endl;
    std::cout << "Using " << num_threads << " threads" << std::endl;
    std::cout << std::endl;

    // Load the graph
    Graph graph = load_undirected_tsv_edgelist_parallel(graph_file, num_threads, true);

    // Print graph statistics
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Graph Statistics" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Number of nodes: " << graph.num_nodes << std::endl;
    std::cout << "Number of edges: " << graph.num_edges << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
