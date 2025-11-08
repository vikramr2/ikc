#!/usr/bin/env python3
"""
Example demonstrating k-core decomposition caching for efficient minimum k-core searches.

When you need to run multiple minimum k-core searches with different parameters,
you can compute the k-core decomposition once and reuse it to avoid redundant computation.
"""

import ikc
import time
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
    print("K-Core Decomposition Caching Example")
    print("=" * 70)
    print()

    # Load the graph
    print(f"Loading graph from: {graph_file}")
    g = ikc.load_graph(str(graph_file), verbose=False)
    print(f"Graph loaded: {g.num_nodes} nodes, {g.num_edges} edges")
    print()

    # =======================================================================
    # Example 1: WITHOUT caching - multiple searches
    # =======================================================================
    print("-" * 70)
    print("Example 1: WITHOUT caching (redundant k-core decomposition)")
    print("-" * 70)
    print("Running 5 minimum k-core searches with different k values...")
    print()

    k_values = [5, 10, 15, 20, 25]

    start_time = time.time()
    results_without_cache = []

    for k in k_values:
        result = g.find_minimum_kcore(k=k)
        results_without_cache.append((k, len(result) if result else 0))
        print(f"  k={k:2d}: minimum k-core has {len(result) if result else 0:4d} nodes")

    time_without_cache = time.time() - start_time
    print(f"\nTotal time WITHOUT caching: {time_without_cache:.3f} seconds")
    print("  (k-core decomposition computed 5 times)")
    print()

    # =======================================================================
    # Example 2: WITH caching - compute once, reuse many times
    # =======================================================================
    print("-" * 70)
    print("Example 2: WITH caching (compute k-core decomposition once)")
    print("-" * 70)
    print("Computing k-core decomposition once...")

    start_time = time.time()

    # Compute k-core decomposition ONCE
    kcore = g.compute_kcore_decomposition()
    decomp_time = time.time() - start_time

    print(f"  Decomposition complete in {decomp_time:.3f} seconds")
    print(f"  Max core number: {kcore.max_core}")
    print()

    print("Running 5 minimum k-core searches with CACHED core numbers...")
    print()

    search_start = time.time()
    results_with_cache = []

    for k in k_values:
        # Reuse the cached core numbers
        result = g.find_minimum_kcore(k=k, core_numbers=kcore.core_numbers)
        results_with_cache.append((k, len(result) if result else 0))
        print(f"  k={k:2d}: minimum k-core has {len(result) if result else 0:4d} nodes")

    search_time = time.time() - search_start
    total_time_with_cache = time.time() - start_time

    print(f"\nDecomposition time: {decomp_time:.3f} seconds")
    print(f"Search time (5 queries): {search_time:.3f} seconds")
    print(f"Total time WITH caching: {total_time_with_cache:.3f} seconds")
    print()

    # =======================================================================
    # Performance comparison
    # =======================================================================
    print("-" * 70)
    print("Performance Comparison")
    print("-" * 70)
    speedup = time_without_cache / total_time_with_cache
    print(f"Time WITHOUT caching: {time_without_cache:.3f} seconds")
    print(f"Time WITH caching:    {total_time_with_cache:.3f} seconds")
    print(f"Speedup:              {speedup:.2f}x faster")
    print()
    print("Key insight: The more queries you run, the better the speedup!")
    print()

    # =======================================================================
    # Example 3: Caching with query-based searches
    # =======================================================================
    print("-" * 70)
    print("Example 3: Caching with query-based searches")
    print("-" * 70)
    print("Finding minimum k-cores around multiple query nodes with caching...")
    print()

    # Pick some query nodes
    if results_with_cache and results_with_cache[0][1] > 0:
        # Get a result to extract query nodes from
        sample_result = g.find_minimum_kcore(k=10, core_numbers=kcore.core_numbers)
        if sample_result and len(sample_result) >= 5:
            query_nodes = sample_result[:5]
            k = 10

            print(f"Query nodes: {query_nodes}")
            print(f"Finding minimum {k}-cores around each node (using cached decomposition)...")
            print()

            for node in query_nodes:
                result = g.find_minimum_kcore_containing_node(
                    query_node=node,
                    k=k,
                    core_numbers=kcore.core_numbers
                )
                if result:
                    print(f"  Node {node:6d}: found {k}-core with {len(result):4d} nodes")
                else:
                    print(f"  Node {node:6d}: not in any {k}-core")

    print()
    print("=" * 70)
    print("Examples completed!")
    print("=" * 70)
    print()
    print("Summary:")
    print("  • compute_kcore_decomposition() computes core numbers once")
    print("  • Pass core_numbers parameter to reuse the decomposition")
    print("  • Especially beneficial for multiple queries on the same graph")
    print("  • Speedup increases with number of queries")
    print()


if __name__ == '__main__':
    main()
