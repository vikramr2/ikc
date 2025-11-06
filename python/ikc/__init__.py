"""
Python wrapper for the Iterative K-Core Clustering (IKC) C++ implementation.
"""
import os
import subprocess
import tempfile
import pandas as pd
from pathlib import Path
from typing import Optional, Union


def _get_ikc_executable():
    """Find the ikc executable in the build directory."""
    # Get the path relative to this file
    current_dir = Path(__file__).parent
    # Go up two levels to the project root, then into build
    ikc_path = current_dir.parent.parent / "build" / "ikc"

    if not ikc_path.exists():
        raise FileNotFoundError(
            f"IKC executable not found at {ikc_path}. "
            "Please build the project first using: cd build && cmake .. && make"
        )

    return str(ikc_path)


class ClusterResult:
    """
    Represents clustering results from the IKC algorithm.
    """

    def __init__(self, data: pd.DataFrame, format_type: str = "csv"):
        """
        Initialize ClusterResult.

        Args:
            data: DataFrame with clustering results
            format_type: Either "csv" (full format) or "tsv" (simple format)
        """
        self.data = data
        self.format_type = format_type

    def save(self, filename: str, tsv: bool = False):
        """
        Save clustering results to a file.

        Args:
            filename: Output file path
            tsv: If True, save as TSV with only node_id and cluster_id (no header)
                 If False, save as CSV with all columns
        """
        if tsv:
            # TSV format: node_id and cluster_id only, no header
            self.data[['node_id', 'cluster_id']].to_csv(
                filename,
                sep='\t',
                index=False,
                header=False
            )
        else:
            # CSV format: all columns with header
            self.data.to_csv(filename, index=False)

        print(f"Results saved to: {filename}")

    def __repr__(self):
        n_clusters = self.data['cluster_id'].nunique()
        n_nodes = len(self.data)
        return f"ClusterResult(nodes={n_nodes}, clusters={n_clusters})"

    def __len__(self):
        """Return number of nodes in the clustering."""
        return len(self.data)

    @property
    def num_clusters(self):
        """Return the number of clusters."""
        return self.data['cluster_id'].nunique()

    @property
    def num_nodes(self):
        """Return the number of nodes."""
        return len(self.data)


class Graph:
    """
    Represents a graph for IKC clustering.
    """

    def __init__(self, graph_file: str):
        """
        Initialize Graph from a TSV edge list file.

        Args:
            graph_file: Path to the graph edge list file (TSV format)
        """
        self.graph_file = str(Path(graph_file).resolve())

        if not Path(self.graph_file).exists():
            raise FileNotFoundError(f"Graph file not found: {self.graph_file}")

    def ikc(self,
            min_k: int = 0,
            num_threads: Optional[int] = None,
            quiet: bool = False) -> ClusterResult:
        """
        Run the Iterative K-Core Clustering algorithm.

        Args:
            min_k: Minimum k value for valid clusters (default: 0)
            num_threads: Number of threads to use (default: hardware concurrency)
            quiet: If True, suppress verbose output from the C++ program

        Returns:
            ClusterResult object containing the clustering results
        """
        ikc_executable = _get_ikc_executable()

        # Create a temporary file for output
        with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as tmp:
            tmp_output = tmp.name

        try:
            # Build command
            cmd = [
                ikc_executable,
                '-e', self.graph_file,
                '-o', tmp_output,
                '-k', str(min_k)
            ]

            if num_threads is not None:
                cmd.extend(['-t', str(num_threads)])

            if quiet:
                cmd.append('-q')

            # Run the IKC algorithm
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True
            )

            if result.returncode != 0:
                raise RuntimeError(
                    f"IKC algorithm failed with return code {result.returncode}\n"
                    f"STDOUT: {result.stdout}\n"
                    f"STDERR: {result.stderr}"
                )

            # Print output if not quiet
            if not quiet and result.stdout:
                print(result.stdout)

            # Read the results
            df = pd.read_csv(tmp_output, header=None,
                           names=['node_id', 'cluster_id', 'k_value', 'modularity'])

            return ClusterResult(df, format_type='csv')

        finally:
            # Clean up temporary file
            if os.path.exists(tmp_output):
                os.remove(tmp_output)

    def __repr__(self):
        return f"Graph(file='{self.graph_file}')"


def load_graph(graph_file: str) -> Graph:
    """
    Load a graph from a TSV edge list file.

    Args:
        graph_file: Path to the graph edge list file (TSV format)

    Returns:
        Graph object ready for clustering

    Example:
        >>> import ikc
        >>> g = ikc.load_graph('net.tsv')
        >>> c = g.ikc(10)
        >>> c.save('out.tsv', tsv=True)
    """
    return Graph(graph_file)


__all__ = ['load_graph', 'Graph', 'ClusterResult']
