#ifndef KCORE_H
#define KCORE_H

#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <unordered_set>
#include "../data_structures/graph.h"

// K-core decomposition result
struct KCoreResult {
    std::vector<uint32_t> core_numbers;  // Core number for each node
    uint32_t max_core;                   // Maximum core number

    KCoreResult(size_t num_nodes) : core_numbers(num_nodes, 0), max_core(0) {}
};

// Compute k-core decomposition using the peeling algorithm
KCoreResult compute_kcore_decomposition(const Graph& graph) {
    KCoreResult result(graph.num_nodes);

    if (graph.num_nodes == 0) return result;

    // Initialize degrees and core numbers
    std::vector<uint32_t> degrees(graph.num_nodes);
    std::vector<bool> removed(graph.num_nodes, false);

    for (uint32_t i = 0; i < graph.num_nodes; i++) {
        degrees[i] = graph.get_degree(i);
    }

    // Create degree bins for efficient peeling
    uint32_t max_degree = *std::max_element(degrees.begin(), degrees.end());
    std::vector<std::vector<uint32_t>> bins(max_degree + 1);

    for (uint32_t i = 0; i < graph.num_nodes; i++) {
        bins[degrees[i]].push_back(i);
    }

    // Peel nodes in order of degree
    uint32_t current_core = 0;
    for (uint32_t bin_idx = 0; bin_idx <= max_degree; bin_idx++) {
        while (!bins[bin_idx].empty()) {
            uint32_t node = bins[bin_idx].back();
            bins[bin_idx].pop_back();

            if (removed[node]) continue;

            result.core_numbers[node] = bin_idx;
            current_core = std::max(current_core, bin_idx);
            removed[node] = true;

            // Update neighbors' degrees
            std::vector<uint32_t> neighbors = graph.get_neighbors(node);
            for (uint32_t neighbor : neighbors) {
                if (!removed[neighbor] && degrees[neighbor] > bin_idx) {
                    degrees[neighbor]--;
                    bins[degrees[neighbor]].push_back(neighbor);
                }
            }
        }
    }

    result.max_core = current_core;
    return result;
}

// Get nodes in k-core (nodes with core number >= k)
std::vector<uint32_t> get_kcore_nodes(const KCoreResult& kcore, uint32_t k) {
    std::vector<uint32_t> kcore_nodes;
    for (uint32_t i = 0; i < kcore.core_numbers.size(); i++) {
        if (kcore.core_numbers[i] >= k) {
            kcore_nodes.push_back(i);
        }
    }
    return kcore_nodes;
}

// Create subgraph from a set of nodes
Graph create_subgraph(const Graph& graph, const std::vector<uint32_t>& nodes) {
    Graph subgraph;

    if (nodes.empty()) return subgraph;

    // Create node mapping
    std::unordered_map<uint32_t, uint32_t> old_to_new;
    subgraph.num_nodes = nodes.size();
    subgraph.id_map.resize(nodes.size());
    subgraph.row_ptr.resize(nodes.size() + 1);
    subgraph.row_ptr[0] = 0;

    for (size_t i = 0; i < nodes.size(); i++) {
        old_to_new[nodes[i]] = i;
        subgraph.id_map[i] = graph.id_map[nodes[i]];
    }

    // Count edges for each node in subgraph
    std::vector<uint32_t> edge_counts(nodes.size(), 0);
    for (size_t i = 0; i < nodes.size(); i++) {
        uint32_t old_node = nodes[i];
        std::vector<uint32_t> neighbors = graph.get_neighbors(old_node);

        for (uint32_t neighbor : neighbors) {
            if (old_to_new.count(neighbor)) {
                edge_counts[i]++;
            }
        }
    }

    // Build row pointers
    for (size_t i = 0; i < nodes.size(); i++) {
        subgraph.row_ptr[i + 1] = subgraph.row_ptr[i] + edge_counts[i];
    }

    // Build column indices
    subgraph.col_idx.resize(subgraph.row_ptr.back());
    std::vector<uint32_t> current_pos = subgraph.row_ptr;
    current_pos.pop_back();

    for (size_t i = 0; i < nodes.size(); i++) {
        uint32_t old_node = nodes[i];
        std::vector<uint32_t> neighbors = graph.get_neighbors(old_node);

        for (uint32_t neighbor : neighbors) {
            if (old_to_new.count(neighbor)) {
                subgraph.col_idx[current_pos[i]++] = old_to_new[neighbor];
            }
        }
    }

    // Update edge count (each undirected edge counted once)
    subgraph.num_edges = subgraph.row_ptr.back() / 2;

    return subgraph;
}

#endif // KCORE_H
