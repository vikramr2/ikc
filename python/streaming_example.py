#!/usr/bin/env python3
"""
Example script demonstrating the StreamingGraph functionality.
Shows how to incrementally update IKC clustering as the graph evolves.
"""
import ikc
import os

# Get the path to the test data
data_dir = os.path.join(os.path.dirname(__file__), '..', 'data')
graph_file = os.path.join(data_dir, 'cit_hepph.tsv')

if not os.path.exists(graph_file):
    print(f"Error: Test data not found at {graph_file}")
    exit(1)

print("=" * 60)
print("Streaming IKC Example")
print("=" * 60)
print()

# === Initial Setup ===
print("Loading graph and computing initial clustering...")
g = ikc.StreamingGraph(graph_file)
print(f"Graph loaded: {g}")
print()

# Run initial IKC clustering
result = g.ikc(min_k=10, verbose=False, progress_bar=True)
print()
print(f"Initial clustering: {result.num_clusters} clusters, {result.num_nodes} nodes")
print(f"Max k-core: {g.max_core}")
print()

# === Incremental Updates ===
print("=" * 60)
print("Testing Incremental Updates")
print("=" * 60)
print()

# Add new edges between existing nodes
print("1. Adding new edges between existing nodes...")
new_edges = [(100, 200), (101, 202), (100, 300)]
result = g.add_edges(new_edges, verbose=False)

# Print update statistics
stats = g.last_update_stats
print(f"   Updated clustering: {result.num_clusters} clusters")
print(f"   Affected nodes: {stats['affected_nodes']}")
print(f"   Invalidated clusters: {stats['invalidated_clusters']}")
print(f"   Valid clusters: {stats['valid_clusters']}")
print(f"   Recomputation time: {stats['recompute_time_ms']:.2f}ms")
print(f"   Total time: {stats['total_time_ms']:.2f}ms")
print()

# Add new isolated nodes
print("2. Adding new isolated nodes...")
new_nodes = [999999, 999998, 999997]
result = g.add_nodes(new_nodes, verbose=False)
print(f"   Updated clustering: {result.num_clusters} clusters")
print(f"   Graph now has {g.num_nodes} nodes")
print()

# Combined update
print("3. Combined update (edges + nodes)...")
result = g.update(
    new_edges=[(500, 600), (501, 601)],
    new_nodes=[888888, 888887],
    verbose=False
)
print(f"   Updated clustering: {result.num_clusters} clusters")
stats = g.last_update_stats
print(f"   Total time: {stats['total_time_ms']:.2f}ms")
print()

# === Batch Mode ===
print("=" * 60)
print("Testing Batch Mode")
print("=" * 60)
print()

print("4. Entering batch mode...")
g.begin_batch()
print(f"   Batch mode: {g.is_batch_mode}")

# Accumulate multiple updates
print("   Adding edges in batch mode (no recomputation)...")
g.add_edges([(700, 800), (701, 801)], verbose=False)
g.add_edges([(702, 802), (703, 803)], verbose=False)
g.add_nodes([777777, 777776], verbose=False)

print(f"   Graph now has {g.num_nodes} nodes (updates pending)")

# Commit all at once
print("   Committing batch...")
result = g.commit_batch(verbose=False)
print(f"   Updated clustering: {result.num_clusters} clusters")
stats = g.last_update_stats
print(f"   Batch commit time: {stats['total_time_ms']:.2f}ms")
print(f"   Batch mode: {g.is_batch_mode}")
print()

# === Save Results ===
print("=" * 60)
print("Saving Results")
print("=" * 60)
print()

output_file = 'streaming_output.csv'
print(f"Saving final clustering to: {output_file}")
g.current_clustering.save(output_file, tsv=False)
print()

# === Summary ===
print("=" * 60)
print("Summary")
print("=" * 60)
print(f"Final graph: {g.num_nodes} nodes, {g.num_edges} edges")
print(f"Final clustering: {result.num_clusters} clusters")
print(f"Max k-core: {g.max_core}")
print()

print("=" * 60)
print("Streaming IKC example completed successfully!")
print("=" * 60)
