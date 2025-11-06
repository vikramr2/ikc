#ifndef IKC_H
#define IKC_H

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <tuple>
#include "../data_structures/graph.h"
#include "kcore.h"
#include "connected_components.h"
#include "clustering_validation.h"

// Cluster information: (nodes, k_value, modularity)
struct Cluster {
    std::vector<uint64_t> nodes;  // Original node IDs
    uint32_t k_value;
    double modularity;

    Cluster(const std::vector<uint64_t>& n, uint32_t k, double m)
        : nodes(n), k_value(k), modularity(m) {}
};

// Remove nodes from graph and return a new compacted graph
// Also updates the node ID mapping
Graph remove_nodes_and_compact(const Graph& graph,
                               const std::unordered_set<uint32_t>& nodes_to_remove,
                               std::vector<uint64_t>& orig_node_ids) {
    std::vector<uint32_t> remaining_nodes;

    for (uint32_t i = 0; i < graph.num_nodes; i++) {
        if (!nodes_to_remove.count(i)) {
            remaining_nodes.push_back(i);
        }
    }

    // Create new graph with remaining nodes
    Graph new_graph = create_subgraph(graph, remaining_nodes);

    // Update orig_node_ids mapping
    std::vector<uint64_t> new_orig_node_ids(remaining_nodes.size());
    for (size_t i = 0; i < remaining_nodes.size(); i++) {
        new_orig_node_ids[i] = orig_node_ids[remaining_nodes[i]];
    }
    orig_node_ids = new_orig_node_ids;

    return new_graph;
}

// Main iterative k-core decomposition algorithm with modularity checking and early stopping
std::vector<Cluster> iterative_kcore_decomposition(Graph graph,
                                                   uint32_t min_k,
                                                   const Graph& orig_graph,
                                                   bool verbose = false) {
    std::vector<Cluster> final_clusters;
    std::vector<uint64_t> singletons;

    // Initialize node ID mapping (maps current graph node IDs to original node IDs)
    std::vector<uint64_t> orig_node_ids = orig_graph.id_map;

    size_t nbr_failed_modularity = 0;
    size_t nbr_failed_k_valid = 0;

    size_t L = orig_graph.num_edges;

    // Continue finding clusters until no nodes left or max_k < min_k
    while (graph.num_nodes > 0) {
        // Compute k-core decomposition
        KCoreResult kcore = compute_kcore_decomposition(graph);
        uint32_t max_k = kcore.max_core;

        if (verbose) {
            std::cout << "Max k-core: " << max_k << ", nodes in graph: " << graph.num_nodes << std::endl;
        }

        // If max_k < min_k, add remaining nodes as singletons
        if (max_k < min_k) {
            if (verbose) {
                std::cout << "Max k < min_k, adding remaining nodes as singletons" << std::endl;
            }

            for (uint32_t node = 0; node < graph.num_nodes; node++) {
                uint64_t orig_node = orig_node_ids[node];
                uint32_t orig_node_idx = 0;

                // Find the node index in original graph
                for (uint32_t i = 0; i < orig_graph.num_nodes; i++) {
                    if (orig_graph.id_map[i] == orig_node) {
                        orig_node_idx = i;
                        break;
                    }
                }

                double modularity = calculate_singleton_modularity(orig_node_idx, orig_graph);
                final_clusters.push_back(Cluster({orig_node}, 0, modularity));
            }

            for (uint64_t node : singletons) {
                final_clusters.push_back(Cluster({node}, 0, 0.0));
            }

            break;
        }

        // Get nodes in max k-core
        std::vector<uint32_t> kcore_nodes = get_kcore_nodes(kcore, max_k);

        if (kcore_nodes.empty()) {
            if (verbose) {
                std::cout << "No nodes in k-core, breaking" << std::endl;
            }
            break;
        }

        if (verbose) {
            std::cout << "K-core nodes: " << kcore_nodes.size() << std::endl;
        }

        // Create subgraph from k-core
        Graph subgraph = create_subgraph(graph, kcore_nodes);

        // Find connected components
        std::vector<std::vector<uint32_t>> components = find_connected_components(subgraph);

        if (verbose) {
            std::cout << "Number of components: " << components.size() << std::endl;
        }

        std::unordered_set<uint32_t> nodes_to_remove;

        // Process each component
        for (const auto& component : components) {
            // Check if component is k-valid
            if (!is_k_valid(component, subgraph, min_k)) {
                if (verbose) {
                    std::cout << "Component failed k-valid check" << std::endl;
                }
                nbr_failed_k_valid++;

                // Add nodes to removal set and singletons
                for (uint32_t subgraph_node : component) {
                    uint32_t graph_node = kcore_nodes[subgraph_node];
                    nodes_to_remove.insert(graph_node);
                    singletons.push_back(orig_node_ids[graph_node]);
                }
                continue;
            }

            // Check modularity (using simplified version that matches Python behavior)
            double modularity = calculate_modularity_simplified(component, orig_graph, orig_node_ids);

            if (modularity <= 0) {
                if (verbose) {
                    std::cout << "Component failed modularity check" << std::endl;
                }
                nbr_failed_modularity++;

                // Add nodes to removal set and singletons
                for (uint32_t subgraph_node : component) {
                    uint32_t graph_node = kcore_nodes[subgraph_node];
                    nodes_to_remove.insert(graph_node);
                    singletons.push_back(orig_node_ids[graph_node]);
                }
                continue;
            }

            // Component is valid, add to clusters
            std::vector<uint64_t> cluster_nodes;
            for (uint32_t subgraph_node : component) {
                uint32_t graph_node = kcore_nodes[subgraph_node];
                cluster_nodes.push_back(orig_node_ids[graph_node]);
                nodes_to_remove.insert(graph_node);
            }

            if (verbose) {
                std::cout << "Adding cluster with " << cluster_nodes.size() << " nodes" << std::endl;
            }

            final_clusters.push_back(Cluster(cluster_nodes, max_k, modularity));
        }

        // Print component size statistics
        if (verbose) {
            auto component_sizes = get_component_sizes(components);
            size_t nbr_large_components = 0;
            for (const auto& [idx, size] : component_sizes) {
                if (size > 100) {
                    nbr_large_components++;
                }
            }
            std::cout << "Components with > 100 nodes: " << nbr_large_components << std::endl;
        }

        // Remove nodes and compact graph
        graph = remove_nodes_and_compact(graph, nodes_to_remove, orig_node_ids);

        if (verbose) {
            std::cout << "Nodes remaining: " << graph.num_nodes << std::endl;
            std::cout << "---" << std::endl;
        }
    }

    if (verbose) {
        std::cout << "Clusters rejected (not k-valid): " << nbr_failed_k_valid << std::endl;
        std::cout << "Clusters rejected (not modular): " << nbr_failed_modularity << std::endl;
        std::cout << "Total clusters: " << final_clusters.size() << std::endl;
    }

    return final_clusters;
}

#endif // IKC_H
