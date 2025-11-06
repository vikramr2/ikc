#ifndef CONNECTED_COMPONENTS_H
#define CONNECTED_COMPONENTS_H

#include <vector>
#include <queue>
#include <unordered_map>
#include "../data_structures/graph.h"

// Find connected components using BFS
std::vector<std::vector<uint32_t>> find_connected_components(const Graph& graph) {
    std::vector<std::vector<uint32_t>> components;

    if (graph.num_nodes == 0) return components;

    std::vector<bool> visited(graph.num_nodes, false);

    for (uint32_t start = 0; start < graph.num_nodes; start++) {
        if (visited[start]) continue;

        // BFS to find component
        std::vector<uint32_t> component;
        std::queue<uint32_t> queue;

        queue.push(start);
        visited[start] = true;

        while (!queue.empty()) {
            uint32_t node = queue.front();
            queue.pop();
            component.push_back(node);

            std::vector<uint32_t> neighbors = graph.get_neighbors(node);
            for (uint32_t neighbor : neighbors) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    queue.push(neighbor);
                }
            }
        }

        components.push_back(component);
    }

    return components;
}

// Get component sizes
std::unordered_map<size_t, size_t> get_component_sizes(const std::vector<std::vector<uint32_t>>& components) {
    std::unordered_map<size_t, size_t> sizes;
    for (size_t i = 0; i < components.size(); i++) {
        sizes[i] = components[i].size();
    }
    return sizes;
}

#endif // CONNECTED_COMPONENTS_H
