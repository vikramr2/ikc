#!/usr/bin/env python3
"""
Example demonstrating maximal k-core search.

Given a query node, find the subgraph containing that node which maximizes k.
This is much simpler and faster than minimum k-core search!
"""

import ikc
from pathlib import Path


def main():
    # Use the citation network dataset
    data_dir = Path(__file__).parent.parent / 'data'
    graph_file = data_dir / 'cit_hepph.tsv'

    if not graph_file.exists():
        print(f"Error: Graph file not found: {graph_file}")
        print("Please ensure the cit_hepph.tsv dataset is in the data/ directory")
        return

    print("=" * 70)
    print("Maximal K-Core Search Example")
    print("=" * 70)
    print()

    # Load the graph
    print(f"Loading graph from: {graph_file}")
    g = ikc.load_graph(str(graph_file), verbose=False)
    print(f"Graph loaded: {g.num_nodes} nodes, {g.num_edges} edges")
    print()

    # Example 1: Find maximal k-core for a single node
    print("-" * 70)
    print("Example 1: Find maximal k-core for a single query node")
    print("-" * 70)

    query_node = 1000
    print(f"Query node: {query_node}")
    result = g.find_maximal_kcore(query_node)

    if result:
        print(f"  Core number: {result['k']}")
        print(f"  Subgraph size: {result['size']} nodes")
        print(f"  First 10 nodes: {result['nodes'][:10]}...")
        print()
        print(f"Interpretation: Node {query_node} belongs to a {result['k']}-core")
        print(f"                (a subgraph where every node has >= {result['k']} neighbors)")
        print(f"                This is the LARGEST k for which node {query_node} is in a k-core")
    else:
        print(f"Node {query_node} not found in graph")
    print()

    # Example 2: Find maximal k-cores for multiple nodes (WITH caching)
    print("-" * 70)
    print("Example 2: Find maximal k-cores for multiple nodes (efficient)")
    print("-" * 70)

    # Compute k-core decomposition once
    print("Computing k-core decomposition once...")
    kcore = g.compute_kcore_decomposition()
    print(f"  Max core in graph: {kcore.max_core}")
    print()

    # Query multiple nodes using cached decomposition
    query_nodes = [100, 500, 1000, 1500, 2000]
    print(f"Finding maximal k-cores for {len(query_nodes)} nodes using cached decomposition:")
    print()

    for node in query_nodes:
        result = g.find_maximal_kcore(node, core_numbers=kcore.core_numbers)
        if result:
            print(f"  Node {node:5d}: {result['k']:3d}-core with {result['size']:5d} nodes")
    print()

    # Example 3: Finding nodes with highest coreness
    print("-" * 70)
    print("Example 3: Finding nodes with highest coreness")
    print("-" * 70)

    # Find nodes with maximum core number
    max_core_nodes = [i for i, cn in enumerate(kcore.core_numbers) if cn == kcore.max_core]
    print(f"Nodes in the {kcore.max_core}-core (highest): {len(max_core_nodes)} nodes")

    if max_core_nodes:
        # Show maximal k-core for a node with highest coreness
        sample_node = max_core_nodes[0]
        result = g.find_maximal_kcore(sample_node, core_numbers=kcore.core_numbers)
        if result:
            print(f"\nMaximal k-core for node {sample_node} (one of the highest coreness nodes):")
            print(f"  Core number: {result['k']}")
            print(f"  Subgraph size: {result['size']} nodes")
    print()

    # Example 4: Use case - finding cohesive communities around nodes
    print("-" * 70)
    print("Example 4: Use case - Finding cohesive communities")
    print("-" * 70)
    print("Use case: Given a set of nodes of interest, find their cohesive communities")
    print()

    nodes_of_interest = [100, 200, 300]
    print(f"Nodes of interest: {nodes_of_interest}")
    print()

    for node in nodes_of_interest:
        result = g.find_maximal_kcore(node, core_numbers=kcore.core_numbers)
        if result:
            print(f"Node {node}:")
            print(f"  - Belongs to {result['k']}-core (max k for this node)")
            print(f"  - Community size: {result['size']} nodes")
            print(f"  - Interpretation: This node is in a tight-knit group where")
            print(f"    everyone has at least {result['k']} connections within the group")
            print()

    print("=" * 70)
    print("Examples completed!")
    print("=" * 70)
    print()
    print("Key insights:")
    print("  • Maximal k-core finds the LARGEST k for which a node is in a k-core")
    print("  • Very efficient: O(n+m) for BFS after k-core decomposition")
    print("  • Perfect for finding cohesive communities around specific nodes")
    print("  • Use cached core_numbers when querying multiple nodes")
    print()


if __name__ == '__main__':
    main()
