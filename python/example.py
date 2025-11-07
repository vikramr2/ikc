#!/usr/bin/env python3
"""
Example script demonstrating the IKC Python wrapper.
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
print("IKC Python Wrapper Example")
print("=" * 60)
print()

# Load the graph
print(f"Loading graph from: {graph_file}")
g = ikc.load_graph(graph_file)
print(f"Graph loaded: {g}")
print()

# Run IKC with min_k=10
print("Running IKC algorithm with min_k=10...")
print("(Note: Add progress_bar=True to see tqdm progress bar)")
print()
clusters = g.ikc(min_k=10, verbose=False, progress_bar=True)
print()

# Print results
print("=" * 60)
print("Results Summary")
print("=" * 60)
print(f"Total nodes in clusters: {clusters.num_nodes}")
print(f"Total clusters found: {clusters.num_clusters}")
print()

# Show first few rows
print("First 10 rows of results (node_id, cluster_id, k_value, modularity):")
for i, row in enumerate(clusters.data[:10]):
    node_id, cluster_id, k_value, modularity = row
    print(f"  {node_id:8d} {cluster_id:6d} {k_value:3d} {modularity:6.3f}")
print()

# Save in TSV format (node_id, cluster_id only, no header)
tsv_output = 'example_output.tsv'
print(f"Saving results in TSV format to: {tsv_output}")
clusters.save(tsv_output, tsv=True)
print()

# Save in CSV format (all columns with header)
csv_output = 'example_output.csv'
print(f"Saving results in CSV format to: {csv_output}")
clusters.save(csv_output, tsv=False)
print()

print("=" * 60)
print("Example completed successfully!")
print("=" * 60)
