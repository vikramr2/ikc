#ifndef MINIMUM_KCORE_SEARCH_H
#define MINIMUM_KCORE_SEARCH_H

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include "../data_structures/graph.h"
#include "kcore.h"

namespace ikc {

/**
 * Result structure for minimum k-core search
 */
struct MinimumKCoreResult {
    std::vector<uint64_t> nodes;        // Nodes in the minimum k-core
    int k_value;                         // The k value
    size_t size;                         // Size of the k-core
    bool found;                          // Whether a solution was found

    MinimumKCoreResult() : k_value(0), size(0), found(false) {}
};

/**
 * Helper function to check if a vertex set forms an s-plex
 * s-plex: each vertex must have degree >= |S| - s within S
 */
bool is_s_plex(const Graph& graph, const std::vector<uint64_t>& vertex_set, int s) {
    std::unordered_set<uint64_t> vertex_set_lookup(vertex_set.begin(), vertex_set.end());

    for (uint64_t v : vertex_set) {
        int degree_in_set = 0;
        const auto& neighbors = graph.get_neighbors(v);

        for (uint64_t neighbor : neighbors) {
            if (vertex_set_lookup.count(neighbor)) {
                degree_in_set++;
            }
        }

        // Each vertex must have degree >= |S| - s
        if (degree_in_set < static_cast<int>(vertex_set.size()) - s) {
            return false;
        }
    }

    return true;
}

/**
 * Helper function to check if vertices can potentially form s-plex
 */
bool can_form_s_plex(const Graph& graph, const std::vector<uint64_t>& vertex_set, int s) {
    if (vertex_set.empty()) return true;

    std::unordered_set<uint64_t> vertex_set_lookup(vertex_set.begin(), vertex_set.end());
    int required_degree = static_cast<int>(vertex_set.size()) - s;

    for (uint64_t v : vertex_set) {
        int degree_in_set = 0;
        const auto& neighbors = graph.get_neighbors(v);

        for (uint64_t neighbor : neighbors) {
            if (vertex_set_lookup.count(neighbor)) {
                degree_in_set++;
            }
        }

        if (degree_in_set < required_degree) {
            return false;
        }
    }

    return true;
}

/**
 * Backtracking search for s-plex of exact target size
 */
bool backtrack_s_plex_search(
    const Graph& graph,
    std::vector<uint64_t>& current_set,
    std::vector<uint64_t>& candidate_set,
    int s,
    size_t target_size,
    std::vector<uint64_t>& result
) {
    // Base case: reached target size
    if (current_set.size() == target_size) {
        if (is_s_plex(graph, current_set, s)) {
            result = current_set;
            return true;
        }
        return false;
    }

    // Pruning: can't reach target size
    if (current_set.size() + candidate_set.size() < target_size) {
        return false;
    }

    // Pruning: already too large
    if (current_set.size() > target_size) {
        return false;
    }

    // Try adding vertices from candidate set
    for (size_t i = 0; i < candidate_set.size(); ++i) {
        uint64_t v = candidate_set[i];

        // Add vertex to current set
        current_set.push_back(v);

        // Create new candidate set (vertices after current position)
        std::vector<uint64_t> new_candidates;
        for (size_t j = i + 1; j < candidate_set.size(); ++j) {
            new_candidates.push_back(candidate_set[j]);
        }

        // Early feasibility check
        if (can_form_s_plex(graph, current_set, s)) {
            if (backtrack_s_plex_search(graph, current_set, new_candidates, s, target_size, result)) {
                return true;
            }
        }

        // Backtrack
        current_set.pop_back();
    }

    return false;
}

/**
 * Find s-plex of exact target_size containing query_node
 */
bool find_s_plex_with_size(
    const Graph& graph,
    uint64_t query_node,
    int s,
    size_t target_size,
    std::vector<uint64_t>& result
) {
    // Initialize with query node
    std::vector<uint64_t> current_set = {query_node};

    // Build candidate set: neighbors of query node + query node itself
    std::unordered_set<uint64_t> candidate_set_lookup;
    candidate_set_lookup.insert(query_node);

    const auto& neighbors = graph.get_neighbors(query_node);
    for (uint64_t neighbor : neighbors) {
        candidate_set_lookup.insert(neighbor);
    }

    std::vector<uint64_t> candidate_set(candidate_set_lookup.begin(), candidate_set_lookup.end());

    // Remove query node from candidates (already in current_set)
    candidate_set.erase(
        std::remove(candidate_set.begin(), candidate_set.end(), query_node),
        candidate_set.end()
    );

    // Backtracking search
    return backtrack_s_plex_search(graph, current_set, candidate_set, s, target_size, result);
}

/**
 * Internal function: Find minimum k-core containing query_node with pre-computed core numbers
 *
 * @param graph The input graph
 * @param query_node The node that must be in the k-core
 * @param k The minimum degree requirement
 * @param core_numbers Pre-computed core numbers (from k-core decomposition)
 * @return MinimumKCoreResult containing the minimum k-core if found
 */
MinimumKCoreResult find_minimum_kcore_containing_node_internal(
    const Graph& graph,
    uint64_t query_node,
    int k,
    const std::vector<uint32_t>& core_numbers
) {
    MinimumKCoreResult result;
    result.k_value = k;
    result.found = false;

    // Check if query_node is a valid internal node ID
    if (query_node >= core_numbers.size()) {
        return result;  // Node doesn't exist
    }

    if (core_numbers[query_node] < static_cast<uint32_t>(k)) {
        return result;  // Node's coreness is less than k
    }

    // Step 2: Iterative search for minimum k-core
    // Try increasing values of s, looking for s-plex of size s+k
    size_t max_s = graph.num_nodes > static_cast<size_t>(k) ?
                   graph.num_nodes - static_cast<size_t>(k) : 0;

    for (size_t s = 1; s <= max_s; ++s) {
        size_t target_size = s + k;

        if (target_size > graph.num_nodes) {
            break;  // Can't form s-plex of this size
        }

        std::vector<uint64_t> s_plex_result;
        bool found = find_s_plex_with_size(graph, query_node, s, target_size, s_plex_result);

        if (found) {
            // Found the minimum k-core!
            // Map to original node IDs
            std::vector<uint64_t> original_ids;
            for (uint64_t internal_id : s_plex_result) {
                if (internal_id < graph.id_map.size()) {
                    original_ids.push_back(graph.id_map[internal_id]);
                } else {
                    original_ids.push_back(internal_id);
                }
            }

            result.nodes = original_ids;
            result.size = original_ids.size();
            result.k_value = k;
            result.found = true;

            return result;
        }
    }

    return result;  // No solution found
}

/**
 * Main function: Find minimum k-core containing query_node
 *
 * This implements the IBB (Iterative Branch-and-Bound) algorithm from:
 * "Efficient Exact Minimum k-Core Search in Real-World Graphs" (CIKM 2023)
 *
 * Key insight: minimum k-core is equivalent to finding the smallest s-plex
 * with size at least s+k, where s-plex means each vertex can miss at most
 * s neighbors within the subgraph.
 *
 * @param graph The input graph
 * @param query_node The node that must be in the k-core
 * @param k The minimum degree requirement
 * @return MinimumKCoreResult containing the minimum k-core if found
 */
MinimumKCoreResult find_minimum_kcore_containing_node(
    const Graph& graph,
    uint64_t query_node,
    int k
) {
    // Compute k-core decomposition and delegate to internal function
    auto kcore_result = compute_kcore_decomposition(graph);
    return find_minimum_kcore_containing_node_internal(graph, query_node, k, kcore_result.core_numbers);
}

/**
 * Overload: Find minimum k-core containing query_node with cached core numbers
 *
 * Use this version when you've already computed k-core decomposition to avoid
 * redundant computation.
 *
 * @param graph The input graph
 * @param query_node The node that must be in the k-core
 * @param k The minimum degree requirement
 * @param core_numbers Pre-computed core numbers (from k-core decomposition)
 * @return MinimumKCoreResult containing the minimum k-core if found
 *
 * Example:
 *   auto kcore = compute_kcore_decomposition(graph);
 *   auto result1 = find_minimum_kcore_containing_node(graph, 42, 5, kcore.core_numbers);
 *   auto result2 = find_minimum_kcore_containing_node(graph, 100, 5, kcore.core_numbers);
 */
MinimumKCoreResult find_minimum_kcore_containing_node(
    const Graph& graph,
    uint64_t query_node,
    int k,
    const std::vector<uint32_t>& core_numbers
) {
    return find_minimum_kcore_containing_node_internal(graph, query_node, k, core_numbers);
}

/**
 * Internal function: Find minimum k-core with pre-computed core numbers
 */
MinimumKCoreResult find_minimum_kcore_internal(
    const Graph& graph,
    int k,
    const std::vector<uint32_t>& core_numbers
) {
    MinimumKCoreResult best_result;
    best_result.k_value = k;
    best_result.found = false;
    best_result.size = std::numeric_limits<size_t>::max();

    // Get all vertices with coreness >= k
    std::vector<uint64_t> candidate_nodes;
    for (size_t i = 0; i < core_numbers.size(); ++i) {
        if (core_numbers[i] >= static_cast<uint32_t>(k)) {
            candidate_nodes.push_back(static_cast<uint64_t>(i));
        }
    }

    if (candidate_nodes.empty()) {
        return best_result;  // No k-core exists
    }

    // Try each candidate as query node (reuse cached core numbers)
    for (uint64_t query_node : candidate_nodes) {
        auto result = find_minimum_kcore_containing_node_internal(graph, query_node, k, core_numbers);

        if (result.found && result.size < best_result.size) {
            best_result = result;
        }
    }

    return best_result;
}

/**
 * Find minimum k-core in the graph (without query node constraint)
 *
 * This tries each vertex as a potential query node and returns the overall
 * minimum k-core found.
 *
 * @param graph The input graph
 * @param k The minimum degree requirement
 * @return MinimumKCoreResult containing the minimum k-core if found
 */
MinimumKCoreResult find_minimum_kcore(const Graph& graph, int k) {
    // Compute k-core decomposition once and reuse for all candidates
    auto kcore_result = compute_kcore_decomposition(graph);
    return find_minimum_kcore_internal(graph, k, kcore_result.core_numbers);
}

/**
 * Overload: Find minimum k-core with cached core numbers
 *
 * Use this version when you've already computed k-core decomposition to avoid
 * redundant computation.
 *
 * @param graph The input graph
 * @param k The minimum degree requirement
 * @param core_numbers Pre-computed core numbers (from k-core decomposition)
 * @return MinimumKCoreResult containing the minimum k-core if found
 *
 * Example:
 *   auto kcore = compute_kcore_decomposition(graph);
 *   auto result1 = find_minimum_kcore(graph, 5, kcore.core_numbers);
 *   auto result2 = find_minimum_kcore(graph, 10, kcore.core_numbers);
 */
MinimumKCoreResult find_minimum_kcore(
    const Graph& graph,
    int k,
    const std::vector<uint32_t>& core_numbers
) {
    return find_minimum_kcore_internal(graph, k, core_numbers);
}

} // namespace ikc

#endif // MINIMUM_KCORE_SEARCH_H
