#!/usr/bin/env python3
"""
Simple test for streaming IKC functionality.
"""
import ikc
import os
import tempfile

def create_test_graph():
    """Create a small test graph file."""
    # Simple graph with a clear k-core structure
    # Triangle (nodes 0,1,2): forms a 2-core
    # Square (nodes 3,4,5,6): forms a 2-core
    # Edge connecting them: (2,3)
    edges = [
        (0, 1),
        (1, 2),
        (2, 0),  # Triangle
        (3, 4),
        (4, 5),
        (5, 6),
        (6, 3),  # Square
        (2, 3),  # Connection
    ]

    fd, path = tempfile.mkstemp(suffix='.tsv', text=True)
    with os.fdopen(fd, 'w') as f:
        for u, v in edges:
            f.write(f"{u}\t{v}\n")

    return path

def test_basic_streaming():
    """Test basic streaming operations."""
    print("Testing basic streaming IKC...")

    # Create test graph
    graph_file = create_test_graph()

    try:
        # Initialize streaming graph
        g = ikc.StreamingGraph(graph_file)
        print(f"  Loaded graph: {g.num_nodes} nodes, {g.num_edges} edges")

        # Initial clustering
        result = g.ikc(min_k=2)
        print(f"  Initial clustering: {result.num_clusters} clusters")

        assert result.num_clusters > 0, "Should have at least one cluster"
        assert g.max_core >= 2, "Should have k-core >= 2"

        # Add edges
        initial_clusters = result.num_clusters
        result = g.add_edges([(0, 3), (1, 4)])  # Connect triangle and square more
        print(f"  After adding edges: {result.num_clusters} clusters")

        stats = g.last_update_stats
        print(f"    Affected nodes: {stats['affected_nodes']}")
        print(f"    Update time: {stats['total_time_ms']:.2f}ms")

        # Add nodes
        result = g.add_nodes([100, 101, 102])
        print(f"  After adding nodes: {g.num_nodes} nodes, {result.num_clusters} clusters")

        # Batch mode
        g.begin_batch()
        assert g.is_batch_mode, "Should be in batch mode"

        g.add_edges([(100, 101)])
        g.add_edges([(101, 102)])

        result = g.commit_batch()
        assert not g.is_batch_mode, "Should have exited batch mode"
        print(f"  After batch commit: {result.num_clusters} clusters")

        print("  ✓ All tests passed!")

    finally:
        # Clean up
        os.unlink(graph_file)

def test_error_handling():
    """Test error handling."""
    print("Testing error handling...")

    graph_file = create_test_graph()

    try:
        g = ikc.StreamingGraph(graph_file)

        # Should fail if ikc() not called
        try:
            g.add_edges([(0, 1)])
            assert False, "Should have raised RuntimeError"
        except RuntimeError as e:
            print(f"  ✓ Correctly raised error: {e}")

        # Now initialize
        g.ikc(min_k=2)

        # This should work now
        result = g.add_edges([(0, 1)])
        print(f"  ✓ Edge addition works after initialization")

        # Test update() with missing nodes
        try:
            g.update(new_edges=[(9999, 8888)])  # Nodes don't exist
            assert False, "Should have raised ValueError"
        except ValueError as e:
            print(f"  ✓ Correctly raised ValueError for missing nodes: {str(e)[:80]}...")

        # Test update() with nodes in new_nodes - should work
        result = g.update(new_edges=[(9999, 8888)], new_nodes=[9999, 8888])
        print(f"  ✓ Update works when all nodes are included")

    finally:
        os.unlink(graph_file)

if __name__ == '__main__':
    print("=" * 60)
    print("Streaming IKC Tests")
    print("=" * 60)
    print()

    test_basic_streaming()
    print()

    test_error_handling()
    print()

    print("=" * 60)
    print("All tests completed!")
    print("=" * 60)
