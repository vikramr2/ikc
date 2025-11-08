"""
Python wrapper for the Iterative K-Core Clustering (IKC) C++ implementation.
"""
from pathlib import Path
from typing import Optional, List, Tuple
import _ikc

try:
    from tqdm import tqdm
    TQDM_AVAILABLE = True
except ImportError:
    TQDM_AVAILABLE = False


class ClusterResult:
    """
    Represents clustering results from the IKC algorithm.
    """

    def __init__(self, clusters: List[_ikc.Cluster]):
        """
        Initialize ClusterResult.

        Args:
            clusters: List of Cluster objects from C++
        """
        self.clusters = clusters
        self._data = None

    @property
    def data(self) -> List[Tuple[int, int, int, float]]:
        """
        Get clustering results as a list of tuples.

        Returns:
            List of (node_id, cluster_id, k_value, modularity) tuples
        """
        if self._data is None:
            self._data = []
            for cluster_idx, cluster in enumerate(self.clusters, start=1):
                for node_id in cluster.nodes:
                    self._data.append((node_id, cluster_idx, cluster.k_value, cluster.modularity))
        return self._data

    def save(self, filename: str, tsv: bool = False):
        """
        Save clustering results to a file.

        Args:
            filename: Output file path
            tsv: If True, save as TSV with only node_id and cluster_id (no header)
                 If False, save as CSV with all columns
        """
        with open(filename, 'w') as f:
            if not tsv:
                # CSV format with header
                f.write("node_id,cluster_id,k_value,modularity\n")

            for cluster_idx, cluster in enumerate(self.clusters, start=1):
                for node_id in cluster.nodes:
                    if tsv:
                        # TSV format: node_id<tab>cluster_id
                        f.write(f"{node_id}\t{cluster_idx}\n")
                    else:
                        # CSV format: all columns
                        f.write(f"{node_id},{cluster_idx},{cluster.k_value},{cluster.modularity}\n")

        print(f"Results saved to: {filename}")

    def __repr__(self):
        n_clusters = len(self.clusters)
        n_nodes = sum(len(cluster.nodes) for cluster in self.clusters)
        return f"ClusterResult(nodes={n_nodes}, clusters={n_clusters})"

    def __len__(self):
        """Return number of nodes in the clustering."""
        return sum(len(cluster.nodes) for cluster in self.clusters)

    @property
    def num_clusters(self):
        """Return the number of clusters."""
        return len(self.clusters)

    @property
    def num_nodes(self):
        """Return the number of nodes."""
        return sum(len(cluster.nodes) for cluster in self.clusters)


class Graph:
    """
    Represents a graph for IKC clustering.
    """

    def __init__(self, graph_file: str, num_threads: Optional[int] = None, verbose: bool = False):
        """
        Initialize Graph from a TSV edge list file.

        Args:
            graph_file: Path to the graph edge list file (TSV format)
            num_threads: Number of threads to use for loading (default: hardware concurrency)
            verbose: If True, print loading progress
        """
        self.graph_file = str(Path(graph_file).resolve())

        if not Path(self.graph_file).exists():
            raise FileNotFoundError(f"Graph file not found: {self.graph_file}")

        # Load graph using C++ binding
        if num_threads is None:
            self._graph = _ikc.load_graph(self.graph_file, verbose=verbose)
        else:
            self._graph = _ikc.load_graph(self.graph_file, num_threads, verbose)

    @property
    def num_nodes(self):
        """Number of nodes in the graph."""
        return self._graph.num_nodes

    @property
    def num_edges(self):
        """Number of edges in the graph."""
        return self._graph.num_edges

    def ikc(self,
            min_k: int = 0,
            verbose: bool = False,
            progress_bar: bool = False) -> ClusterResult:
        """
        Run the Iterative K-Core Clustering algorithm.

        Args:
            min_k: Minimum k value for valid clusters (default: 0)
            verbose: If True, print algorithm progress
            progress_bar: If True, display tqdm progress bar tracking k-core decomposition
                         from initial max k (0%) to min_k (100%)

        Returns:
            ClusterResult object containing the clustering results
        """
        # Setup progress bar if requested
        pbar = None
        initial_k = [None]  # Use list to allow modification in nested function

        def progress_callback(current_k: int):
            """Callback function to update progress bar"""
            if pbar is None:
                return

            # Set initial k on first call
            if initial_k[0] is None:
                initial_k[0] = current_k
                # Set up progress bar range from initial_k to min_k
                pbar.total = max(1, initial_k[0] - min_k)
                pbar.n = 0
                pbar.refresh()
            else:
                # Calculate progress: how far we've gone from initial_k toward min_k
                progress = initial_k[0] - current_k
                pbar.n = progress
                pbar.set_postfix({'current_k': current_k})
                pbar.refresh()

        if progress_bar:
            if not TQDM_AVAILABLE:
                import warnings
                warnings.warn("tqdm is not installed. Install it with 'pip install tqdm' to use progress_bar feature.")
                callback = None
            else:
                pbar = tqdm(total=1, desc="IKC Progress", unit="k-core levels")
                callback = progress_callback
        else:
            callback = None

        try:
            # Run IKC algorithm using C++ binding
            clusters = _ikc.run_ikc(self._graph, min_k, self._graph, verbose, callback)
        finally:
            if pbar is not None:
                pbar.close()

        return ClusterResult(clusters)

    def __repr__(self):
        return f"Graph(file='{self.graph_file}', nodes={self.num_nodes}, edges={self.num_edges})"


def load_graph(graph_file: str, num_threads: Optional[int] = None, verbose: bool = False) -> Graph:
    """
    Load a graph from a TSV edge list file.

    Args:
        graph_file: Path to the graph edge list file (TSV format)
        num_threads: Number of threads to use for loading (default: hardware concurrency)
        verbose: If True, print loading progress

    Returns:
        Graph object ready for clustering

    Example:
        >>> import ikc
        >>> g = ikc.load_graph('net.tsv')
        >>> c = g.ikc(10)
        >>> c.save('out.tsv', tsv=True)
    """
    return Graph(graph_file, num_threads, verbose)


class StreamingGraph:
    """
    Graph with support for incremental IKC updates.
    """

    def __init__(self, graph_file: str, num_threads: Optional[int] = None, verbose: bool = False):
        """
        Initialize StreamingGraph from a TSV edge list file.

        Args:
            graph_file: Path to the graph edge list file (TSV format)
            num_threads: Number of threads to use for loading (default: hardware concurrency)
            verbose: If True, print loading progress
        """
        self.graph_file = str(Path(graph_file).resolve())

        if not Path(self.graph_file).exists():
            raise FileNotFoundError(f"Graph file not found: {self.graph_file}")

        # Load graph using C++ binding
        if num_threads is None:
            self._graph = _ikc.load_graph(self.graph_file, verbose=verbose)
        else:
            self._graph = _ikc.load_graph(self.graph_file, num_threads, verbose)

        # StreamingIKC instance (created after initial clustering)
        self._streaming_ikc = None
        self._min_k = None
        self._current_result = None

    @property
    def num_nodes(self):
        """Number of nodes in the graph."""
        if self._streaming_ikc is not None:
            return self._streaming_ikc.get_num_nodes()
        return self._graph.num_nodes

    @property
    def num_edges(self):
        """Number of edges in the graph."""
        if self._streaming_ikc is not None:
            return self._streaming_ikc.get_num_edges()
        return self._graph.num_edges

    def ikc(self,
            min_k: int = 0,
            verbose: bool = False,
            progress_bar: bool = False) -> ClusterResult:
        """
        Run the Iterative K-Core Clustering algorithm and initialize streaming state.

        Args:
            min_k: Minimum k value for valid clusters (default: 0)
            verbose: If True, print algorithm progress
            progress_bar: If True, display tqdm progress bar

        Returns:
            ClusterResult object containing the clustering results
        """
        # Setup progress bar if requested
        pbar = None
        initial_k = [None]

        def progress_callback(current_k: int):
            """Callback function to update progress bar"""
            if pbar is None:
                return

            if initial_k[0] is None:
                initial_k[0] = current_k
                pbar.total = max(1, initial_k[0] - min_k)
                pbar.n = 0
                pbar.refresh()
            else:
                progress = initial_k[0] - current_k
                pbar.n = progress
                pbar.set_postfix({'current_k': current_k})
                pbar.refresh()

        if progress_bar:
            if not TQDM_AVAILABLE:
                import warnings
                warnings.warn("tqdm is not installed. Install it with 'pip install tqdm' to use progress_bar feature.")
                callback = None
            else:
                pbar = tqdm(total=1, desc="IKC Progress", unit="k-core levels")
                callback = progress_callback
        else:
            callback = None

        try:
            # Initialize streaming IKC
            self._min_k = min_k
            self._streaming_ikc = _ikc.StreamingIKC(self._graph, min_k)

            # Run initial clustering
            clusters = self._streaming_ikc.initial_clustering(verbose, callback)
            self._current_result = ClusterResult(clusters)

        finally:
            if pbar is not None:
                pbar.close()

        return self._current_result

    def add_edges(self, edges: List[Tuple[int, int]], verbose: bool = False) -> ClusterResult:
        """
        Add new edges to the graph and update clustering incrementally.

        Args:
            edges: List of (node_id, node_id) tuples representing edges
            verbose: If True, print update progress

        Returns:
            Updated ClusterResult

        Raises:
            RuntimeError: If ikc() hasn't been called yet
        """
        if self._streaming_ikc is None:
            raise RuntimeError("Must call ikc() before add_edges(). Streaming state not initialized.")

        clusters = self._streaming_ikc.add_edges(edges, recompute=True, verbose=verbose)
        self._current_result = ClusterResult(clusters)
        return self._current_result

    def add_nodes(self, nodes: List[int], verbose: bool = False) -> ClusterResult:
        """
        Add new isolated nodes to the graph.

        Args:
            nodes: List of node IDs
            verbose: If True, print update progress

        Returns:
            Updated ClusterResult

        Raises:
            RuntimeError: If ikc() hasn't been called yet
        """
        if self._streaming_ikc is None:
            raise RuntimeError("Must call ikc() before add_nodes(). Streaming state not initialized.")

        clusters = self._streaming_ikc.add_nodes(nodes, recompute=True, verbose=verbose)
        self._current_result = ClusterResult(clusters)
        return self._current_result

    def update(self,
               new_edges: Optional[List[Tuple[int, int]]] = None,
               new_nodes: Optional[List[int]] = None,
               verbose: bool = False) -> ClusterResult:
        """
        Add both edges and nodes in a single update.

        More efficient than separate add_edges() and add_nodes() calls.

        Important: All nodes referenced in new_edges must either already exist in the graph
        or be included in new_nodes.

        Args:
            new_edges: List of (node_id, node_id) tuples (optional)
            new_nodes: List of node IDs (optional)
            verbose: If True, print update progress

        Returns:
            Updated ClusterResult

        Raises:
            RuntimeError: If ikc() hasn't been called yet
            ValueError: If an edge references a node that doesn't exist and isn't in new_nodes

        Example:
            # Correct: all edge nodes are included
            g.update(new_edges=[(100, 200)], new_nodes=[100, 200])

            # Error: nodes 100, 200 don't exist and aren't in new_nodes
            g.update(new_edges=[(100, 200)])  # ValueError!
        """
        if self._streaming_ikc is None:
            raise RuntimeError("Must call ikc() before update(). Streaming state not initialized.")

        edges = new_edges if new_edges is not None else []
        nodes = new_nodes if new_nodes is not None else []

        try:
            clusters = self._streaming_ikc.update(edges, nodes, verbose)
            self._current_result = ClusterResult(clusters)
            return self._current_result
        except Exception as e:
            # Re-raise with more helpful context
            error_msg = str(e)
            if "references non-existent node" in error_msg:
                raise ValueError(error_msg) from e
            raise

    def begin_batch(self):
        """
        Enter batch mode - accumulate updates without recomputation.

        Use commit_batch() to apply all pending updates at once.

        Raises:
            RuntimeError: If ikc() hasn't been called yet
        """
        if self._streaming_ikc is None:
            raise RuntimeError("Must call ikc() before begin_batch(). Streaming state not initialized.")

        self._streaming_ikc.begin_batch()

    def commit_batch(self, verbose: bool = False) -> ClusterResult:
        """
        Exit batch mode and apply all pending updates.

        Args:
            verbose: If True, print update progress

        Returns:
            Updated ClusterResult

        Raises:
            RuntimeError: If ikc() hasn't been called yet
        """
        if self._streaming_ikc is None:
            raise RuntimeError("Must call ikc() before commit_batch(). Streaming state not initialized.")

        clusters = self._streaming_ikc.commit_batch(verbose)
        self._current_result = ClusterResult(clusters)
        return self._current_result

    @property
    def current_clustering(self) -> Optional[ClusterResult]:
        """Get current clustering without recomputation."""
        return self._current_result

    @property
    def last_update_stats(self) -> dict:
        """
        Statistics from last update operation.

        Returns dict with:
            - affected_nodes: number of nodes with changed core numbers
            - invalidated_clusters: number of clusters that were recomputed
            - valid_clusters: number of clusters unchanged
            - merge_candidates: number of potential cluster merges detected
            - recompute_time_ms: time spent in localized recomputation
            - total_time_ms: total update time

        Returns:
            Dictionary of update statistics, or None if no updates have been performed
        """
        if self._streaming_ikc is None:
            return None

        stats = self._streaming_ikc.get_last_stats()
        return {
            'affected_nodes': stats.affected_nodes,
            'invalidated_clusters': stats.invalidated_clusters,
            'valid_clusters': stats.valid_clusters,
            'merge_candidates': stats.merge_candidates,
            'recompute_time_ms': stats.recompute_time_ms,
            'total_time_ms': stats.total_time_ms
        }

    @property
    def max_core(self) -> Optional[int]:
        """Get current maximum core number."""
        if self._streaming_ikc is None:
            return None
        return self._streaming_ikc.get_max_core()

    @property
    def is_batch_mode(self) -> bool:
        """Check if currently in batch mode."""
        if self._streaming_ikc is None:
            return False
        return self._streaming_ikc.is_batch_mode()

    def __repr__(self):
        if self._streaming_ikc is not None:
            return f"StreamingGraph(file='{self.graph_file}', nodes={self.num_nodes}, edges={self.num_edges}, clusters={len(self._current_result.clusters) if self._current_result else 0})"
        return f"StreamingGraph(file='{self.graph_file}', nodes={self.num_nodes}, edges={self.num_edges})"


__all__ = ['load_graph', 'Graph', 'ClusterResult', 'StreamingGraph']
