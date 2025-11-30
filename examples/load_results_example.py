#!/usr/bin/env python3
"""
Example demonstrating how to load pre-computed IKC results from CSV
to avoid recomputing the clustering.
"""
import ikc
import os

print("=" * 60)
print("Loading Pre-computed IKC Results Example")
print("=" * 60)
print()

# Example 1: Load clustering results
print("Example 1: Load pre-computed clustering results")
print("-" * 60)

# Assuming you have already run IKC and saved the results:
# g = ikc.load_graph('network.tsv')
# clusters = g.ikc(min_k=10)
# clusters.save('results.csv')

# Now you can load the results without recomputing:
csv_file = 'example_output.csv'
if os.path.exists(csv_file):
    clusters = ikc.ClusterResult.load(csv_file)
    print()
    print(f"Number of clusters: {clusters.num_clusters}")
    print(f"Total nodes in clusters: {clusters.num_nodes}")
    print()

    # You can access the data just like a freshly computed result
    print("First 5 rows of data:")
    for i, (node_id, cluster_id, k_value, modularity) in enumerate(clusters.data[:5]):
        print(f"  Node {node_id}: cluster={cluster_id}, k={k_value}, modularity={modularity:.3f}")
    print()
else:
    print(f"File {csv_file} not found. Run example.py first to generate results.")
    print()

# Example 2: Save and load k-core decomposition
print("Example 2: Save and load k-core decomposition")
print("-" * 60)

# Get the path to the test data
data_dir = os.path.join(os.path.dirname(__file__), '..', 'data')
graph_file = os.path.join(data_dir, 'cit_hepph.tsv')

if os.path.exists(graph_file):
    # Load graph
    g = ikc.load_graph(graph_file)
    print(f"Loaded graph: {g.num_nodes} nodes, {g.num_edges} edges")
    print()

    # Compute k-core decomposition once
    print("Computing k-core decomposition...")
    kcore = g.compute_kcore_decomposition()
    print(f"Max core: {kcore.max_core}")
    print()

    # Save it for later use
    kcore_file = 'kcore_decomposition.csv'
    kcore.save(kcore_file)
    print()

    # Later, you can load it without recomputing
    print("Loading k-core decomposition from file...")
    loaded_kcore = ikc.KCoreDecomposition.load(kcore_file, num_nodes=g.num_nodes)
    print()

    # Use the loaded core numbers for maximal k-core search
    query_node = 100
    if query_node < g.num_nodes:
        result = g.find_maximal_kcore(query_node, core_numbers=loaded_kcore.core_numbers)
        if result:
            print(f"Maximal k-core for node {query_node}:")
            print(f"  k-value: {result['k']}")
            print(f"  size: {result['size']} nodes")
            print()

    # Clean up example file
    if os.path.exists(kcore_file):
        os.remove(kcore_file)
        print(f"Cleaned up {kcore_file}")
else:
    print(f"Graph file not found: {graph_file}")

print()
print("=" * 60)
print("Example completed!")
print("=" * 60)
