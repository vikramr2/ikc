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


__all__ = ['load_graph', 'Graph', 'ClusterResult']
