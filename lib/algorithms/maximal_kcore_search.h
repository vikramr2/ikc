#ifndef MAXIMAL_KCORE_SEARCH_H
#define MAXIMAL_KCORE_SEARCH_H

#include <vector>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include "../data_structures/graph.h"
#include "kcore.h"
#include "connected_components.h"

namespace ikc {

/**
 * Result structure for maximal k-core search
 */
struct MaximalKCoreResult {
    std::vector<uint64_t> nodes;        // Nodes in the maximal k-core
    uint32_t k_value;                   // The k value (core number)
    size_t size;                        // Size of the k-core
    bool found;                         // Whether a solution was found

    MaximalKCoreResult() : k_value(0), size(0), found(false) {}
};

/**
 * Find the maximal k-core containing a query node with pre-computed core numbers
 *
 * This finds the subgraph containing the query node that maximizes k, where
 * k is the core number of the query node. This is the largest k for which
 * the query node belongs to a k-core.
 *
 * Algorithm:
 * 1. Get the core number of query node (this is the maximal k)
 * 2. Extract all nodes with core number >= k
 * 3. Find the connected component containing the query node
 *
 * Time complexity: O(n + m) for BFS to find connected component
 *
 * @param graph The input graph
 * @param query_node The node whose maximal k-core we want to find
 * @param core_numbers Pre-computed core numbers (from k-core decomposition)
 * @return MaximalKCoreResult containing the maximal k-core
 */
MaximalKCoreResult find_maximal_kcore_internal(
    const Graph& graph,
    uint64_t query_node,
    const std::vector<uint32_t>& core_numbers
) {
    MaximalKCoreResult result;
    result.found = false;

    // Check if query_node is valid
    if (query_node >= core_numbers.size()) {
        return result;  // Node doesn't exist
    }

    // Get the core number of the query node - this is the maximal k
    uint32_t k = core_numbers[query_node];
    result.k_value = k;

    if (k == 0) {
        // Isolated node - return just this node
        result.nodes.push_back(query_node < graph.id_map.size() ?
                              graph.id_map[query_node] : query_node);
        result.size = 1;
        result.found = true;
        return result;
    }

    // Extract all nodes with core number >= k (these form the k-core)
    std::unordered_set<uint64_t> kcore_nodes;
    for (uint64_t i = 0; i < core_numbers.size(); ++i) {
        if (core_numbers[i] >= k) {
            kcore_nodes.insert(i);
        }
    }

    // Find connected component containing query_node within the k-core
    // Using BFS
    std::unordered_set<uint64_t> visited;
    std::queue<uint64_t> queue;
    std::vector<uint64_t> component;

    queue.push(query_node);
    visited.insert(query_node);

    while (!queue.empty()) {
        uint64_t current = queue.front();
        queue.pop();
        component.push_back(current);

        // Visit neighbors that are in the k-core
        const auto& neighbors = graph.get_neighbors(current);
        for (uint64_t neighbor : neighbors) {
            if (kcore_nodes.count(neighbor) && !visited.count(neighbor)) {
                visited.insert(neighbor);
                queue.push(neighbor);
            }
        }
    }

    // Map to original node IDs
    std::vector<uint64_t> original_ids;
    for (uint64_t internal_id : component) {
        if (internal_id < graph.id_map.size()) {
            original_ids.push_back(graph.id_map[internal_id]);
        } else {
            original_ids.push_back(internal_id);
        }
    }

    result.nodes = original_ids;
    result.size = original_ids.size();
    result.found = true;

    return result;
}

/**
 * Find the maximal k-core containing a query node
 *
 * This finds the subgraph containing the query node that maximizes k.
 * The maximal k is the core number of the query node.
 *
 * @param graph The input graph
 * @param query_node The node whose maximal k-core we want to find
 * @return MaximalKCoreResult containing the maximal k-core
 *
 * Example:
 *   auto result = find_maximal_kcore(graph, 42);
 *   if (result.found) {
 *       cout << "Node 42 is in a " << result.k_value << "-core ";
 *       cout << "with " << result.size << " nodes" << endl;
 *   }
 */
MaximalKCoreResult find_maximal_kcore(
    const Graph& graph,
    uint64_t query_node
) {
    // Compute k-core decomposition
    auto kcore_result = compute_kcore_decomposition(graph);
    return find_maximal_kcore_internal(graph, query_node, kcore_result.core_numbers);
}

/**
 * Overload: Find maximal k-core with cached core numbers
 *
 * Use this version when you've already computed k-core decomposition to avoid
 * redundant computation.
 *
 * @param graph The input graph
 * @param query_node The node whose maximal k-core we want to find
 * @param core_numbers Pre-computed core numbers (from k-core decomposition)
 * @return MaximalKCoreResult containing the maximal k-core
 *
 * Example:
 *   auto kcore = compute_kcore_decomposition(graph);
 *   auto result1 = find_maximal_kcore(graph, 42, kcore.core_numbers);
 *   auto result2 = find_maximal_kcore(graph, 100, kcore.core_numbers);
 */
MaximalKCoreResult find_maximal_kcore(
    const Graph& graph,
    uint64_t query_node,
    const std::vector<uint32_t>& core_numbers
) {
    return find_maximal_kcore_internal(graph, query_node, core_numbers);
}

} // namespace ikc

#endif // MAXIMAL_KCORE_SEARCH_H
