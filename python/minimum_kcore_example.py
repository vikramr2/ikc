#!/usr/bin/env python3
"""
Example script demonstrating minimum k-core search functionality.

This shows how to use the new minimum k-core search features to find
the smallest k-core in a graph, both with and without a query node.
"""

import ikc
import sys
from pathlib import Path


def main():
    # Use the citation network dataset
    data_dir = Path(__file__).parent.parent / 'data'
    graph_file = data_dir / 'cit_hepph.tsv'

    if not graph_file.exists():
        print(f"Error: Graph file not found: {graph_file}")
        print("Please ensure the cit_hepph.tsv dataset is in the data/ directory")
        sys.exit(1)

    print("=" * 70)
    print("Minimum K-Core Search Example")
    print("=" * 70)
    print()

    # Load the graph
    print(f"Loading graph from: {graph_file}")
    g = ikc.load_graph(str(graph_file), verbose=True)
    print(f"Graph loaded: {g.num_nodes} nodes, {g.num_edges} edges")
    print()

    # Example 1: Find minimum k-core without query node
    print("-" * 70)
    print("Example 1: Find minimum k-core in the entire graph")
    print("-" * 70)

    k = 10
    print(f"Searching for minimum {k}-core...")
    min_kcore = g.find_minimum_kcore(k=k)

    if min_kcore:
        print(f"✓ Found minimum {k}-core with {len(min_kcore)} nodes")
        print(f"  Nodes: {min_kcore[:10]}{'...' if len(min_kcore) > 10 else ''}")

        # Verify it's a k-core (optional - for demonstration)
        print(f"  Note: Each node in this subgraph has at least {k} neighbors within it")
    else:
        print(f"✗ No {k}-core exists in this graph")
    print()

    # Example 2: Find minimum k-core containing a specific node
    print("-" * 70)
    print("Example 2: Find minimum k-core containing a query node")
    print("-" * 70)

    # Pick a query node (let's use the first node from the previous result)
    if min_kcore and len(min_kcore) > 0:
        query_node = min_kcore[0]
        print(f"Query node: {query_node}")
        print(f"Searching for minimum {k}-core containing node {query_node}...")

        query_result = g.find_minimum_kcore_containing_node(query_node=query_node, k=k)

        if query_result:
            print(f"✓ Found minimum {k}-core containing node {query_node}")
            print(f"  Size: {len(query_result)} nodes")
            print(f"  Nodes: {query_result[:10]}{'...' if len(query_result) > 10 else ''}")
        else:
            print(f"✗ Node {query_node} is not in any {k}-core")
    print()

    # Example 3: Compare different k values
    print("-" * 70)
    print("Example 3: Comparing minimum k-cores for different k values")
    print("-" * 70)

    k_values = [5, 10, 15, 20]
    print(f"Comparing k values: {k_values}")
    print()

    results = []
    for k_val in k_values:
        result = g.find_minimum_kcore(k=k_val)
        if result:
            results.append((k_val, len(result)))
            print(f"  k={k_val:2d}: minimum k-core has {len(result):4d} nodes")
        else:
            print(f"  k={k_val:2d}: no k-core exists")

    print()
    if results:
        print("Observation: As k increases, minimum k-core size typically increases")
        print("             (more constraints = larger minimum solution)")
    print()

    # Example 4: Use case - Finding tight-knit communities around specific nodes
    print("-" * 70)
    print("Example 4: Finding tight-knit communities (use case)")
    print("-" * 70)

    print("Use case: Finding the most condensed community around influential nodes")
    print()

    # Pick a few query nodes (using nodes from the graph)
    if min_kcore and len(min_kcore) >= 3:
        query_nodes = min_kcore[:3]
        k = 8

        print(f"Finding minimum {k}-cores around {len(query_nodes)} nodes:")
        for node in query_nodes:
            result = g.find_minimum_kcore_containing_node(query_node=node, k=k)
            if result:
                print(f"  Node {node:6d}: found {k}-core with {len(result):4d} nodes")
            else:
                print(f"  Node {node:6d}: not in any {k}-core")

    print()
    print("=" * 70)
    print("Examples completed!")
    print("=" * 70)
    print()
    print("Key insights:")
    print("  • Minimum k-core search finds the SMALLEST k-core (by node count)")
    print("  • Useful for finding condensed cohesive communities")
    print("  • Query-based search finds minimum k-core around specific nodes")
    print("  • Applications: protein complexes, social groups, recommendation systems")
    print()


if __name__ == '__main__':
    main()