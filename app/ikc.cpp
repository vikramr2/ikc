#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#include "../lib/io/graph_io.h"
#include "../lib/data_structures/graph.h"
#include "../lib/algorithms/ikc.h"

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " -e <graph_file.tsv> -o <output.csv> [-k <min_k>] [-t <num_threads>] [-q] [--tsv]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -e <graph_file.tsv>  Path to input graph edge list (TSV format)" << std::endl;
    std::cerr << "  -o <output.csv>      Path to output CSV file" << std::endl;
    std::cerr << "  -k <min_k>           Minimum k value for valid clusters (default: 0)" << std::endl;
    std::cerr << "  -t <num_threads>     Number of threads (default: hardware concurrency)" << std::endl;
    std::cerr << "  -q                   Quiet mode (suppress verbose output)" << std::endl;
    std::cerr << "  --tsv                Output as TSV (node_id cluster_id) without header" << std::endl;
}

void write_clusters_to_csv(const std::string& output_file, const std::vector<Cluster>& clusters, bool tsv_format = false) {
    std::ofstream out(output_file);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open output file: " << output_file << std::endl;
        return;
    }

    size_t cluster_index = 0;
    for (const auto& cluster : clusters) {
        cluster_index++;
        for (uint64_t node : cluster.nodes) {
            if (tsv_format) {
                // TSV format: node_id<tab>cluster_id
                out << node << "\t" << cluster_index << "\n";
            } else {
                // CSV format: node_id,cluster_index,k_value,modularity
                out << node << "," << cluster_index << "," << cluster.k_value << "," << cluster.modularity << "\n";
            }
        }
    }

    out.close();
    std::cout << "Results written to: " << output_file << std::endl;
}

int main(int argc, char* argv[]) {
    std::string graph_file;
    std::string output_file;
    uint32_t min_k = 0;
    int num_threads = std::thread::hardware_concurrency();
    bool quiet = false;
    bool tsv_format = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-e" && i + 1 < argc) {
            graph_file = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "-k" && i + 1 < argc) {
            min_k = std::stoi(argv[++i]);
        } else if (arg == "-t" && i + 1 < argc) {
            num_threads = std::stoi(argv[++i]);
        } else if (arg == "-q") {
            quiet = true;
        } else if (arg == "--tsv") {
            tsv_format = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    // Validate required arguments
    if (graph_file.empty() || output_file.empty()) {
        std::cerr << "Error: Both -e (input file) and -o (output file) are required." << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    if (!quiet) {
        std::cout << "========================================" << std::endl;
        std::cout << "Iterative K-Core Clustering (IKC)" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Input graph: " << graph_file << std::endl;
        std::cout << "Output file: " << output_file << std::endl;
        std::cout << "Minimum k: " << min_k << std::endl;
        std::cout << "Threads: " << num_threads << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
    }

    // Load the graph
    if (!quiet) {
        std::cout << "Loading graph..." << std::endl;
    }
    Graph graph = load_undirected_tsv_edgelist_parallel(graph_file, num_threads, !quiet);

    if (graph.num_nodes == 0) {
        std::cerr << "Error: Failed to load graph or graph is empty." << std::endl;
        return 1;
    }

    if (!quiet) {
        std::cout << std::endl;
        std::cout << "Graph loaded successfully!" << std::endl;
        std::cout << "Nodes: " << graph.num_nodes << std::endl;
        std::cout << "Edges: " << graph.num_edges << std::endl;
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Running IKC algorithm..." << std::endl;
        std::cout << "========================================" << std::endl;
    }

    // Run IKC algorithm
    Graph orig_graph = graph;  // Keep original graph for modularity calculations
    std::vector<Cluster> clusters = iterative_kcore_decomposition(graph, min_k, orig_graph, !quiet);

    if (!quiet) {
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "IKC Complete!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total clusters found: " << clusters.size() << std::endl;
    }

    // Write results to CSV or TSV
    write_clusters_to_csv(output_file, clusters, tsv_format);

    if (!quiet) {
        std::cout << "========================================" << std::endl;
        std::cout << "Done!" << std::endl;
        std::cout << "========================================" << std::endl;
    }

    return 0;
}
