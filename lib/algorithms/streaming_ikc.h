#ifndef STREAMING_IKC_H
#define STREAMING_IKC_H

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <queue>
#include <algorithm>
#include <iostream>
#include "../data_structures/graph.h"
#include "ikc.h"
#include "kcore.h"

/**
 * Statistics about a streaming update operation
 */
struct UpdateStats {
    size_t affected_nodes = 0;
    size_t invalidated_clusters = 0;
    size_t valid_clusters = 0;
    size_t merge_candidates = 0;
    double recompute_time_ms = 0.0;
    double total_time_ms = 0.0;

    UpdateStats() = default;
};

/**
 * Streaming IKC state - maintains graph, clustering, and core numbers
 * for efficient incremental updates
 */
class StreamingIKC {
private:
    Graph graph_;                                    // Current graph
    Graph orig_graph_;                               // Original graph (for modularity)
    std::vector<Cluster> clusters_;                  // Current clustering
    std::vector<uint32_t> core_numbers_;             // Core number for each node
    std::vector<uint32_t> cluster_assignment_;       // cluster_id for each node
    uint32_t min_k_;                                 // Minimum k threshold
    uint32_t max_core_;                              // Current maximum core number
    UpdateStats last_stats_;                         // Statistics from last update
    bool batch_mode_;                                // Whether in batch mode
    std::vector<std::pair<uint64_t, uint64_t>> pending_edges_;  // Pending edges in batch mode
    std::vector<uint64_t> pending_nodes_;            // Pending nodes in batch mode

    /**
     * Update core numbers incrementally after adding edges
     * Based on Sariyüce et al. (2013) algorithm
     */
    std::unordered_set<uint32_t> update_core_numbers_incremental(
        const std::vector<std::pair<uint32_t, uint32_t>>& new_edges) {

        std::unordered_set<uint32_t> affected_nodes;

        if (new_edges.empty()) {
            return affected_nodes;
        }

        // Find the minimum k value among endpoints of new edges
        uint32_t k_max = 0;
        for (const auto& [u, v] : new_edges) {
            k_max = std::max(k_max, std::max(core_numbers_[u], core_numbers_[v]));
        }

        // Candidates: nodes that could be promoted
        std::unordered_set<uint32_t> candidates;
        for (const auto& [u, v] : new_edges) {
            if (core_numbers_[u] >= k_max) {
                candidates.insert(u);
            }
            if (core_numbers_[v] >= k_max) {
                candidates.insert(v);
            }
        }

        // Priority queue: process nodes by increasing core number
        using PQElement = std::pair<uint32_t, uint32_t>; // (core_number, node)
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;

        for (uint32_t node : candidates) {
            pq.push({core_numbers_[node], node});
        }

        std::unordered_set<uint32_t> visited;

        while (!pq.empty()) {
            auto [k_current, v] = pq.top();
            pq.pop();

            if (visited.count(v)) {
                continue;
            }
            visited.insert(v);

            // Check if v should be promoted
            // Count neighbors in (k_current + 1)-core or higher
            uint32_t neighbors_in_higher_core = 0;
            std::vector<uint32_t> neighbors = graph_.get_neighbors(v);

            for (uint32_t w : neighbors) {
                if (core_numbers_[w] >= k_current + 1) {
                    neighbors_in_higher_core++;
                }
            }

            // Promotion condition: degree in (k+1)-core >= k+1
            if (neighbors_in_higher_core >= k_current + 1) {
                // Promote v to next k-core
                core_numbers_[v] = k_current + 1;
                affected_nodes.insert(v);
                max_core_ = std::max(max_core_, core_numbers_[v]);

                // Neighbors at k_current might also be promotable
                for (uint32_t w : neighbors) {
                    if (core_numbers_[w] == k_current && !visited.count(w)) {
                        pq.push({core_numbers_[w], w});
                    }
                }
            }
        }

        return affected_nodes;
    }

    /**
     * Detect which clusters are invalidated by affected nodes
     */
    void detect_invalid_clusters(
        const std::unordered_set<uint32_t>& affected_nodes,
        std::vector<size_t>& valid_cluster_indices,
        std::vector<size_t>& invalid_cluster_indices,
        std::unordered_set<uint32_t>& nodes_to_recompute) {

        valid_cluster_indices.clear();
        invalid_cluster_indices.clear();
        nodes_to_recompute.clear();

        for (size_t cluster_idx = 0; cluster_idx < clusters_.size(); ++cluster_idx) {
            const Cluster& cluster = clusters_[cluster_idx];

            // Check if cluster has any affected nodes
            bool has_affected = false;
            for (uint64_t orig_node_id : cluster.nodes) {
                // Convert original node ID to internal ID
                if (graph_.node_map.count(orig_node_id)) {
                    uint32_t internal_id = graph_.node_map.at(orig_node_id);
                    if (affected_nodes.count(internal_id)) {
                        has_affected = true;
                        break;
                    }
                }
            }

            if (!has_affected) {
                // Cluster completely unaffected
                valid_cluster_indices.push_back(cluster_idx);
                continue;
            }

            // Cluster has affected nodes - check if still k-valid
            uint32_t k = cluster.k_value;
            bool k_valid = true;

            std::unordered_set<uint64_t> cluster_node_set(cluster.nodes.begin(),
                                                           cluster.nodes.end());

            for (uint64_t orig_node_id : cluster.nodes) {
                if (!graph_.node_map.count(orig_node_id)) {
                    k_valid = false;
                    break;
                }

                uint32_t internal_id = graph_.node_map.at(orig_node_id);

                // Count internal degree
                uint32_t internal_degree = 0;
                std::vector<uint32_t> neighbors = graph_.get_neighbors(internal_id);

                for (uint32_t neighbor_internal_id : neighbors) {
                    uint64_t neighbor_orig_id = graph_.id_map[neighbor_internal_id];
                    if (cluster_node_set.count(neighbor_orig_id)) {
                        internal_degree++;
                    }
                }

                if (internal_degree < k) {
                    k_valid = false;
                    break;
                }
            }

            if (!k_valid) {
                // Cluster is invalid - needs recomputation
                invalid_cluster_indices.push_back(cluster_idx);
                for (uint64_t orig_node_id : cluster.nodes) {
                    if (graph_.node_map.count(orig_node_id)) {
                        nodes_to_recompute.insert(graph_.node_map.at(orig_node_id));
                    }
                }
                continue;
            }

            // Check for potential merges (external nodes promoted to this k-core)
            bool has_merge_candidates = false;

            for (uint64_t orig_node_id : cluster.nodes) {
                if (!graph_.node_map.count(orig_node_id)) continue;

                uint32_t internal_id = graph_.node_map.at(orig_node_id);
                std::vector<uint32_t> neighbors = graph_.get_neighbors(internal_id);

                for (uint32_t neighbor_internal_id : neighbors) {
                    uint64_t neighbor_orig_id = graph_.id_map[neighbor_internal_id];

                    // Check if neighbor is outside cluster but has high enough core
                    if (!cluster_node_set.count(neighbor_orig_id) &&
                        core_numbers_[neighbor_internal_id] >= k) {
                        has_merge_candidates = true;
                        break;
                    }
                }

                if (has_merge_candidates) break;
            }

            if (has_merge_candidates) {
                // Needs recomputation due to potential merge
                invalid_cluster_indices.push_back(cluster_idx);
                for (uint64_t orig_node_id : cluster.nodes) {
                    if (graph_.node_map.count(orig_node_id)) {
                        nodes_to_recompute.insert(graph_.node_map.at(orig_node_id));
                    }
                }

                // Also include high-core neighbors
                for (uint64_t orig_node_id : cluster.nodes) {
                    if (!graph_.node_map.count(orig_node_id)) continue;

                    uint32_t internal_id = graph_.node_map.at(orig_node_id);
                    std::vector<uint32_t> neighbors = graph_.get_neighbors(internal_id);

                    for (uint32_t neighbor_internal_id : neighbors) {
                        if (core_numbers_[neighbor_internal_id] >= k) {
                            nodes_to_recompute.insert(neighbor_internal_id);
                        }
                    }
                }
            } else {
                // Still valid
                valid_cluster_indices.push_back(cluster_idx);
            }
        }
    }

    /**
     * Recompute clusters for affected regions
     */
    std::vector<Cluster> recompute_affected_clusters(
        const std::unordered_set<uint32_t>& nodes_to_recompute,
        bool verbose = false) {

        if (nodes_to_recompute.empty()) {
            return {};
        }

        if (verbose) {
            std::cout << "Recomputing " << nodes_to_recompute.size()
                      << " affected nodes..." << std::endl;
        }

        // Create subgraph of affected region
        std::vector<uint32_t> nodes_vec(nodes_to_recompute.begin(),
                                        nodes_to_recompute.end());
        Graph subgraph = create_subgraph(graph_, nodes_vec);

        // Run IKC on subgraph
        std::vector<Cluster> new_clusters = iterative_kcore_decomposition(
            subgraph, min_k_, orig_graph_, verbose, nullptr);

        if (verbose) {
            std::cout << "Recomputation produced " << new_clusters.size()
                      << " clusters" << std::endl;
        }

        return new_clusters;
    }

    /**
     * Update cluster assignments after recomputation
     */
    void update_cluster_assignments() {
        cluster_assignment_.assign(graph_.num_nodes, UINT32_MAX);

        for (size_t cluster_idx = 0; cluster_idx < clusters_.size(); ++cluster_idx) {
            const Cluster& cluster = clusters_[cluster_idx];

            for (uint64_t orig_node_id : cluster.nodes) {
                if (graph_.node_map.count(orig_node_id)) {
                    uint32_t internal_id = graph_.node_map.at(orig_node_id);
                    cluster_assignment_[internal_id] = cluster_idx;
                }
            }
        }
    }

public:
    /**
     * Constructor - initializes with a graph
     */
    StreamingIKC(const Graph& graph, uint32_t min_k = 0)
        : graph_(graph), orig_graph_(graph), min_k_(min_k), max_core_(0),
          batch_mode_(false) {
        // Initialize cluster assignment to -1 (unassigned)
        cluster_assignment_.resize(graph_.num_nodes, UINT32_MAX);
    }

    /**
     * Run initial IKC clustering
     */
    std::vector<Cluster> initial_clustering(bool verbose = false,
                                           std::function<void(uint32_t)> progress_callback = nullptr) {

        if (verbose) {
            std::cout << "Running initial IKC clustering..." << std::endl;
        }

        // Run standard IKC algorithm
        clusters_ = iterative_kcore_decomposition(graph_, min_k_, orig_graph_,
                                                  verbose, progress_callback);

        // Compute core numbers for the original graph
        KCoreResult kcore = compute_kcore_decomposition(graph_);
        core_numbers_ = kcore.core_numbers;
        max_core_ = kcore.max_core;

        // Update cluster assignments
        update_cluster_assignments();

        if (verbose) {
            std::cout << "Initial clustering complete: " << clusters_.size()
                      << " clusters, max_core=" << max_core_ << std::endl;
        }

        return clusters_;
    }

    /**
     * Add edges to the graph and update clustering
     *
     * @param edges Vector of (original_node_id, original_node_id) pairs
     * @param recompute If false, defer clustering update (batch mode)
     * @return Updated clustering (or current clustering if recompute=false)
     */
    std::vector<Cluster> add_edges(const std::vector<std::pair<uint64_t, uint64_t>>& edges,
                                   bool recompute = true,
                                   bool verbose = false) {

        if (batch_mode_) {
            // In batch mode, just accumulate edges
            pending_edges_.insert(pending_edges_.end(), edges.begin(), edges.end());
            return clusters_;
        }

        if (edges.empty()) {
            return clusters_;
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // Convert original node IDs to internal IDs
        std::vector<std::pair<uint32_t, uint32_t>> internal_edges;
        for (const auto& [u_orig, v_orig] : edges) {
            // Ensure nodes exist
            if (!graph_.node_map.count(u_orig) || !graph_.node_map.count(v_orig)) {
                if (verbose) {
                    std::cout << "Warning: edge (" << u_orig << ", " << v_orig
                              << ") references non-existent nodes" << std::endl;
                }
                continue;
            }

            uint32_t u = graph_.node_map[u_orig];
            uint32_t v = graph_.node_map[v_orig];
            internal_edges.push_back({u, v});
        }

        if (internal_edges.empty()) {
            return clusters_;
        }

        // Add edges to graph
        add_edges_batch(graph_, internal_edges);

        if (!recompute) {
            return clusters_;
        }

        // Update core numbers incrementally
        auto affected_nodes = update_core_numbers_incremental(internal_edges);

        auto recompute_start = std::chrono::high_resolution_clock::now();

        // Detect invalid clusters
        std::vector<size_t> valid_indices, invalid_indices;
        std::unordered_set<uint32_t> nodes_to_recompute;

        detect_invalid_clusters(affected_nodes, valid_indices, invalid_indices,
                               nodes_to_recompute);

        // If no clusters affected, we're done
        if (invalid_indices.empty() && nodes_to_recompute.empty()) {
            last_stats_.affected_nodes = affected_nodes.size();
            last_stats_.invalidated_clusters = 0;
            last_stats_.valid_clusters = clusters_.size();
            last_stats_.merge_candidates = 0;
            last_stats_.recompute_time_ms = 0.0;

            auto end_time = std::chrono::high_resolution_clock::now();
            last_stats_.total_time_ms = std::chrono::duration<double, std::milli>(
                end_time - start_time).count();

            return clusters_;
        }

        // Recompute affected clusters
        std::vector<Cluster> new_clusters = recompute_affected_clusters(
            nodes_to_recompute, verbose);

        auto recompute_end = std::chrono::high_resolution_clock::now();

        // Merge results: keep valid clusters + new clusters
        std::vector<Cluster> updated_clusters;
        for (size_t idx : valid_indices) {
            updated_clusters.push_back(clusters_[idx]);
        }
        updated_clusters.insert(updated_clusters.end(),
                              new_clusters.begin(), new_clusters.end());

        clusters_ = updated_clusters;
        update_cluster_assignments();

        // Update statistics
        last_stats_.affected_nodes = affected_nodes.size();
        last_stats_.invalidated_clusters = invalid_indices.size();
        last_stats_.valid_clusters = valid_indices.size();
        last_stats_.merge_candidates = nodes_to_recompute.size();
        last_stats_.recompute_time_ms = std::chrono::duration<double, std::milli>(
            recompute_end - recompute_start).count();

        auto end_time = std::chrono::high_resolution_clock::now();
        last_stats_.total_time_ms = std::chrono::duration<double, std::milli>(
            end_time - start_time).count();

        if (verbose) {
            std::cout << "Update complete: " << last_stats_.affected_nodes
                      << " affected nodes, " << last_stats_.invalidated_clusters
                      << " invalidated clusters, " << clusters_.size()
                      << " total clusters" << std::endl;
        }

        return clusters_;
    }

    /**
     * Add nodes to the graph (initially isolated)
     *
     * @param nodes Vector of original node IDs
     * @param recompute If false, defer clustering update
     * @return Updated clustering (or current clustering if recompute=false)
     */
    std::vector<Cluster> add_nodes(const std::vector<uint64_t>& nodes,
                                   bool recompute = true,
                                   bool verbose = false) {

        if (batch_mode_) {
            // In batch mode, just accumulate nodes
            pending_nodes_.insert(pending_nodes_.end(), nodes.begin(), nodes.end());
            return clusters_;
        }

        if (nodes.empty()) {
            return clusters_;
        }

        // Add nodes to graph (initially isolated, so core number = 0)
        for (uint64_t orig_id : nodes) {
            if (!graph_.node_map.count(orig_id)) {
                graph_.add_node(orig_id);
                core_numbers_.push_back(0);  // Isolated node has core 0
                cluster_assignment_.push_back(UINT32_MAX);  // Unassigned
            }
        }

        // Isolated nodes don't affect existing clusters
        // They become singleton clusters with k=0
        if (recompute) {
            for (uint64_t orig_id : nodes) {
                if (graph_.node_map.count(orig_id)) {
                    uint32_t internal_id = graph_.node_map.at(orig_id);

                    // Only create singleton if not already in a cluster
                    if (cluster_assignment_[internal_id] == UINT32_MAX) {
                        clusters_.push_back(Cluster({orig_id}, 0, 0.0));
                    }
                }
            }
            update_cluster_assignments();
        }

        if (verbose) {
            std::cout << "Added " << nodes.size() << " isolated nodes" << std::endl;
        }

        return clusters_;
    }

    /**
     * Add both edges and nodes in a single update
     */
    std::vector<Cluster> update(const std::vector<std::pair<uint64_t, uint64_t>>& edges,
                               const std::vector<uint64_t>& nodes,
                               bool verbose = false) {

        // Validate that all edge endpoints exist or will be added
        if (!edges.empty()) {
            std::unordered_set<uint64_t> nodes_to_add(nodes.begin(), nodes.end());

            for (const auto& [u, v] : edges) {
                bool u_exists = graph_.node_map.count(u) || nodes_to_add.count(u);
                bool v_exists = graph_.node_map.count(v) || nodes_to_add.count(v);

                if (!u_exists || !v_exists) {
                    std::string missing_nodes;
                    if (!u_exists) missing_nodes += std::to_string(u);
                    if (!u_exists && !v_exists) missing_nodes += ", ";
                    if (!v_exists) missing_nodes += std::to_string(v);

                    throw std::invalid_argument(
                        "Edge (" + std::to_string(u) + ", " + std::to_string(v) +
                        ") references non-existent node(s): " + missing_nodes +
                        ". All nodes in new_edges must either exist in the graph or be included in new_nodes."
                    );
                }
            }
        }

        // Add nodes first (without recomputation)
        if (!nodes.empty()) {
            add_nodes(nodes, false, verbose);
        }

        // Then add edges (with recomputation)
        if (!edges.empty()) {
            auto result = add_edges(edges, true, verbose);

            // Ensure any nodes that weren't clustered get singleton clusters
            for (uint64_t orig_id : nodes) {
                if (graph_.node_map.count(orig_id)) {
                    uint32_t internal_id = graph_.node_map.at(orig_id);

                    // Create singleton if not already in a cluster
                    if (cluster_assignment_[internal_id] == UINT32_MAX) {
                        clusters_.push_back(Cluster({orig_id}, 0, 0.0));
                    }
                }
            }

            // Update assignments after adding singletons
            if (!nodes.empty()) {
                update_cluster_assignments();
            }

            return clusters_;
        }

        // If only nodes were added, recompute now
        return add_nodes({}, true, verbose);
    }

    /**
     * Enter batch mode - accumulate updates without recomputation
     */
    void begin_batch() {
        batch_mode_ = true;
        pending_edges_.clear();
        pending_nodes_.clear();
    }

    /**
     * Exit batch mode and apply all pending updates
     */
    std::vector<Cluster> commit_batch(bool verbose = false) {
        if (!batch_mode_) {
            if (verbose) {
                std::cout << "Warning: not in batch mode" << std::endl;
            }
            return clusters_;
        }

        batch_mode_ = false;

        if (verbose) {
            std::cout << "Committing batch: " << pending_edges_.size()
                      << " edges, " << pending_nodes_.size() << " nodes" << std::endl;
        }

        // Apply all pending updates
        return update(pending_edges_, pending_nodes_, verbose);
    }

    // Getters
    const std::vector<Cluster>& get_clusters() const { return clusters_; }
    const std::vector<uint32_t>& get_core_numbers() const { return core_numbers_; }
    const Graph& get_graph() const { return graph_; }
    const UpdateStats& get_last_stats() const { return last_stats_; }
    size_t get_num_nodes() const { return graph_.num_nodes; }
    size_t get_num_edges() const { return graph_.num_edges; }
    uint32_t get_max_core() const { return max_core_; }
    bool is_batch_mode() const { return batch_mode_; }
};

#endif // STREAMING_IKC_H
