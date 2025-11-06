#ifndef CLUSTERING_VALIDATION_H
#define CLUSTERING_VALIDATION_H

#include <vector>
#include <unordered_set>
#include <cmath>
#include "../data_structures/graph.h"

// Check if a component is k-valid (all nodes have degree >= k within the component)
bool is_k_valid(const std::vector<uint32_t>& component, const Graph& subgraph, uint32_t k) {
    std::unordered_set<uint32_t> component_nodes(component.begin(), component.end());

    for (uint32_t node : component) {
        uint32_t degree_in_component = 0;
        std::vector<uint32_t> neighbors = subgraph.get_neighbors(node);

        for (uint32_t neighbor : neighbors) {
            if (component_nodes.count(neighbor)) {
                degree_in_component++;
            }
        }

        if (degree_in_component < k) {
            return false;
        }
    }

    return true;
}

// Calculate modularity for a component
// Modularity = ls/L - (ds/(2*L))^2
// where:
//   ls = number of edges in the cluster
//   L = total number of edges in the original graph
//   ds = sum of degrees of nodes in the cluster (in the original graph)
double calculate_modularity(const std::vector<uint32_t>& component,
                           const Graph& orig_graph,
                           const std::vector<uint64_t>& node_id_map_to_orig) {
    // NOTE: The Python code has this function return a constant POSITIVE_VALUE = 1
    // This means modularity checking is effectively disabled in the original implementation
    // Keeping the actual calculation here for completeness, but can be simplified if needed

    size_t L = orig_graph.num_edges;
    if (L == 0) return 0.0;

    // Count edges within the cluster (ls)
    std::unordered_set<uint32_t> component_set(component.begin(), component.end());
    size_t ls = 0;

    for (uint32_t node : component) {
        std::vector<uint32_t> neighbors = orig_graph.get_neighbors(node);
        for (uint32_t neighbor : neighbors) {
            if (component_set.count(neighbor) && node < neighbor) {
                ls++;
            }
        }
    }

    // Sum of degrees (ds)
    uint64_t ds = 0;
    for (uint32_t node : component) {
        ds += orig_graph.get_degree(node);
    }

    // Calculate modularity
    double modularity = (double)ls / L - std::pow((double)ds / (2.0 * L), 2);

    return modularity;
}

// Simplified modularity that matches Python behavior (always returns 1)
double calculate_modularity_simplified(const std::vector<uint32_t>& component,
                                      const Graph& orig_graph,
                                      const std::vector<uint64_t>& node_id_map_to_orig) {
    return 1.0;  // Matches Python POSITIVE_VALUE behavior
}

// Calculate modularity for a single node (used when k < min_k)
double calculate_singleton_modularity(uint32_t node, const Graph& orig_graph) {
    size_t L = orig_graph.num_edges;
    if (L == 0) return 0.0;

    uint32_t degree = orig_graph.get_degree(node);
    return -1.0 * std::pow((double)degree / (2.0 * L), 2);
}

#endif // CLUSTERING_VALIDATION_H
