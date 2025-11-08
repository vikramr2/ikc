# IKC - Iterative K-Core Clustering

Fast C++ implementation of Iterative K-Core Clustering with Python wrapper.

## Project Structure

```
ikc/
├── app/                     # C++ application
├── lib/                     # C++ libraries
│   ├── algorithms/          # Core IKC algorithms
│   ├── data_structures/     # Graph data structures
│   └── io/                  # Graph I/O utilities
├── python/                  # Python wrapper
│   ├── ikc/                 # Python package
│   └── bindings.cpp         # pybind11 C++ bindings
├── examples/                # Example Python scripts
├── data/                    # Test datasets
└── build/                   # Build directory
```

## Building the C++ Executable

```bash
cd build
cmake ..
make
```

The compiled executable will be at `build/ikc`.

## C++ Command-Line Usage

```bash
./ikc -e <graph_file.tsv> -o <output.csv> [-k <min_k>] [-t <num_threads>] [-q] [--tsv]
```

### Options

- `-e <graph_file.tsv>` - Path to input graph edge list (TSV format)
- `-o <output.csv>` - Path to output file
- `-k <min_k>` - Minimum k value for valid clusters (default: 0)
- `-t <num_threads>` - Number of threads (default: hardware concurrency)
- `-q` - Quiet mode (suppress verbose output)
- `--tsv` - Output as TSV (node_id cluster_id) without header

### Examples

```bash
# Run with default settings (CSV output with all columns)
./ikc -e data/cit_hepph.tsv -o output.csv

# Run with min_k=10 and TSV output
./ikc -e data/cit_hepph.tsv -o output.tsv -k 10 --tsv

# Run with 8 threads in quiet mode
./ikc -e data/cit_hepph.tsv -o output.csv -k 10 -t 8 -q
```

### Output Formats

**CSV format** (default):
```
node_id,cluster_id,k_value,modularity
3306,1,30,1.0
9803315,1,30,1.0
```

**TSV format** (with `--tsv` flag):
```
3306	1
9803315	1
```

## Python Wrapper

The Python wrapper uses **pybind11** to directly bind the C++ code, providing:
- Fast performance (no subprocess overhead)
- Clean Python API
- No heavy dependencies (no pandas required)

### Installation

**Option 1: Install from PyPI (once published):**

```bash
pip install ikc
```

**Option 2: Install from source:**

**Install dependencies:**

```bash
pip install -r requirements.txt
```

**Install the Python package:**

```bash
pip install -e .
```

This will automatically compile the C++ extension and install the Python package.

Alternatively, install in one step (pip will handle dependencies):

```bash
pip install -e .
```

**Verify installation:**

```bash
python3 -c "import ikc; print('IKC wrapper installed successfully!')"
```

### Python Usage

```python
import ikc

# Load a graph from a TSV edge list file
g = ikc.load_graph('net.tsv')

# Run the IKC algorithm with min_k=10
c = g.ikc(10)

# Save results as TSV (node_id, cluster_id, no header)
c.save('out.tsv', tsv=True)

# Or save as CSV with all columns (node_id, cluster_id, k_value, modularity)
c.save('out.csv', tsv=False)

# Access clustering information
print(f"Number of clusters: {c.num_clusters}")
print(f"Number of nodes: {c.num_nodes}")

# Access the underlying data as list of tuples
print(c.data[:10])  # First 10 rows
```

### Python API Reference

#### `ikc.load_graph(graph_file, num_threads=None, verbose=False)`

Load a graph from a TSV edge list file.

**Parameters:**
- `graph_file` (str): Path to the graph edge list file (TSV format)
- `num_threads` (int, optional): Number of threads for loading (default: hardware concurrency)
- `verbose` (bool): Print loading progress (default: False)

**Returns:**
- `Graph`: Graph object ready for clustering

#### `Graph.ikc(min_k=0, verbose=False, progress_bar=False)`

Run the Iterative K-Core Clustering algorithm.

**Parameters:**
- `min_k` (int): Minimum k value for valid clusters (default: 0)
- `verbose` (bool): Print algorithm progress (default: False)
- `progress_bar` (bool): Display tqdm progress bar tracking k-core decomposition from initial max k (0%) to min_k (100%) (default: False)

**Returns:**
- `ClusterResult`: Object containing the clustering results

**Progress Bar:**
When `progress_bar=True`, a tqdm progress bar displays the k-core decomposition progress:
- **Initial max k-core value** → **0% progress**
- **Target min_k value** → **100% progress**

For example, if you run `ikc(min_k=10)` and the first core found is k=40:
- k=40 → 0% progress (start)
- k=30 → 33% progress
- k=20 → 66% progress
- k=10 → 100% progress (complete)

The progress bar shows the current k value and processing speed.

#### `ClusterResult.save(filename, tsv=False)`

Save clustering results to a file.

**Parameters:**
- `filename` (str): Output file path
- `tsv` (bool): If True, save as TSV with only node_id and cluster_id (no header). If False, save as CSV with all columns (default: False)

#### `ClusterResult` Properties

- `num_clusters`: Number of clusters found
- `num_nodes`: Number of nodes in the clustering
- `data`: List of tuples `(node_id, cluster_id, k_value, modularity)` for all clustered nodes
- `clusters`: List of C++ Cluster objects with `nodes`, `k_value`, and `modularity` attributes

### Python Example

```python
import ikc

# Load graph with 4 threads
g = ikc.load_graph('data/cit_hepph.tsv', num_threads=4)
print(g)  # Graph(file='...', nodes=34546, edges=420877)

# Run IKC with min_k=10 and progress bar
clusters = g.ikc(min_k=10, progress_bar=True)
# Output: IKC Progress: 100%|██████████| 20/20 [00:00<00:00, 84.99k-core levels/s, current_k=10]

# Print summary
print(clusters)  # ClusterResult(nodes=34546, clusters=27639)

# Save in TSV format
clusters.save('output.tsv', tsv=True)

# Save in CSV format with all columns
clusters.save('output.csv', tsv=False)

# Access the data (first 5 rows)
for row in clusters.data[:5]:
    node_id, cluster_id, k_value, modularity = row
    print(f"Node {node_id} in cluster {cluster_id}")

# Get cluster statistics
print(f"Total clusters: {clusters.num_clusters}")
print(f"Total nodes: {clusters.num_nodes}")
```

### Run Example Scripts

```bash
# Basic IKC example
python3 examples/example.py

# Streaming IKC example
python3 examples/streaming_example.py

# Maximal k-core search example
python3 examples/maximal_kcore_example.py
```

---

## Maximal K-Core Search

**⚠️ Beta Feature**: This functionality is currently in beta and under active development.

The maximal k-core search finds the largest k-core containing a given query node. This is useful for discovering cohesive communities around specific nodes of interest.

### Overview

Given a query node, the algorithm finds the subgraph containing that node which maximizes k, where k is the core number of the query node. This represents the most cohesive community that the node belongs to.

**Key Properties:**
- The maximal k is simply the core number of the query node
- Very efficient: O(n+m) for BFS after k-core decomposition
- Can cache k-core decomposition for multiple queries
- Perfect for finding tight-knit communities around specific nodes

### Python API

#### Basic Usage

```python
import ikc

# Load graph
g = ikc.load_graph('network.tsv')

# Find maximal k-core for a query node
result = g.find_maximal_kcore(query_node=1000)

if result:
    print(f"Node {query_node} belongs to a {result['k']}-core")
    print(f"Community size: {result['size']} nodes")
    print(f"Nodes in community: {result['nodes']}")
```

#### Efficient Multi-Query Pattern (with Caching)

```python
# Compute k-core decomposition once
kcore = g.compute_kcore_decomposition()
print(f"Max core in graph: {kcore.max_core}")

# Query multiple nodes using cached decomposition
query_nodes = [100, 500, 1000, 1500, 2000]

for node in query_nodes:
    result = g.find_maximal_kcore(node, core_numbers=kcore.core_numbers)
    if result:
        print(f"Node {node}: {result['k']}-core with {result['size']} nodes")
```

### API Reference

#### `Graph.compute_kcore_decomposition() -> KCoreResult`

Compute k-core decomposition and return core numbers for all nodes.

**Returns:**
- `KCoreResult`: Object with `core_numbers` (list of core numbers) and `max_core` (maximum core value)

#### `Graph.find_maximal_kcore(query_node, core_numbers=None) -> dict`

Find the maximal k-core containing a query node.

**Parameters:**
- `query_node` (int): The node whose maximal k-core to find
- `core_numbers` (list, optional): Pre-computed core numbers from `compute_kcore_decomposition()` (recommended for multiple queries)

**Returns:**
- Dictionary with keys:
  - `nodes`: List of node IDs in the maximal k-core
  - `k`: The k value (core number of the query node)
  - `size`: Number of nodes in the k-core
- Returns `None` if node not found in graph

### Example Use Cases

**Finding Cohesive Communities:**
```python
nodes_of_interest = [100, 200, 300]
kcore = g.compute_kcore_decomposition()

for node in nodes_of_interest:
    result = g.find_maximal_kcore(node, core_numbers=kcore.core_numbers)
    if result:
        print(f"Node {node} is in a {result['k']}-core")
        print(f"  Community size: {result['size']} nodes")
        print(f"  Interpretation: This node is in a tight-knit group where")
        print(f"  everyone has at least {result['k']} connections within the group")
```

**Finding High-Coreness Nodes:**
```python
kcore = g.compute_kcore_decomposition()

# Find nodes with maximum core number
max_core_nodes = [i for i, cn in enumerate(kcore.core_numbers)
                  if cn == kcore.max_core]
print(f"Nodes in the {kcore.max_core}-core: {len(max_core_nodes)} nodes")

# Examine their communities
sample_node = max_core_nodes[0]
result = g.find_maximal_kcore(sample_node, core_numbers=kcore.core_numbers)
print(f"Community size for highest coreness node: {result['size']} nodes")
```

See `examples/maximal_kcore_example.py` for complete examples.

---

## Requirements

### C++
- CMake 3.10+
- C++17 compatible compiler
- OpenMP support (for parallel graph loading)

### Python
- Python 3.7+
- pybind11 >= 2.6.0 (automatically installed with `pip install`)
- tqdm >= 4.0.0 (for progress bar feature, automatically installed with `pip install`)

## Input Format

The input graph file should be a TSV (tab-separated) edge list with two columns:
```
node1	node2
node3	node4
...
```

No header required. Nodes can be any integer IDs.

---

## Streaming IKC (Incremental Updates)

The streaming IKC implementation allows you to efficiently update clustering as your graph evolves over time, without full recomputation.

### Overview

The streaming algorithm:
- Starts with an initial graph and IKC clustering
- Adds new edges and nodes incrementally
- Efficiently updates clustering using localized recomputation
- Provides 10-100x speedup for sparse updates vs full recomputation

### Algorithm Summary

The streaming algorithm consists of three main phases:

1. **Incremental Core Number Update** - Based on Sariyüce et al. (2013), when edges are added:
   - Core numbers can only increase (never decrease)
   - Only nodes near added edges need checking
   - Uses priority queue for efficient promotion

2. **Cluster Invalidation Detection** - For each existing cluster:
   - **Valid**: No affected nodes → cluster unchanged
   - **Invalid (k-validity)**: Internal degrees drop below k → needs recomputation
   - **Invalid (merge)**: External nodes promoted to same k-core → potential merge

3. **Localized Recomputation** - Extract affected subgraph and run standard IKC on just that region

### Python API

#### Basic Usage

```python
import ikc

# Load graph and compute initial clustering
g = ikc.StreamingGraph('network.tsv')
result = g.ikc(min_k=10)

print(f"Initial: {result.num_clusters} clusters")

# Add edges incrementally
result = g.add_edges([(100, 200), (101, 202)])
print(f"After update: {result.num_clusters} clusters")

# View update statistics
stats = g.last_update_stats
print(f"Affected nodes: {stats['affected_nodes']}")
print(f"Update time: {stats['total_time_ms']}ms")
```

#### Batch Mode (Recommended for Multiple Updates)

```python
# Accumulate updates without recomputation
g.begin_batch()

g.add_edges([(1, 2), (3, 4)])
g.add_edges([(5, 6), (7, 8)])
g.add_nodes([100, 101, 102])

# Apply all updates at once
result = g.commit_batch()
```

#### Combined Updates

```python
# More efficient than separate calls
result = g.update(
    new_edges=[(1, 2), (3, 4)],
    new_nodes=[1, 2, 3, 4]  # Include all nodes referenced in edges
)
```

**Important Note**: When using `update()`, all nodes referenced in `new_edges` must either:
1. Already exist in the graph, OR
2. Be included in the `new_nodes` list

For example, this will raise an error:
```python
# ERROR: Nodes 100 and 200 don't exist and aren't in new_nodes
result = g.update(new_edges=[(100, 200)])  # ValueError!
```

Correct usage:
```python
# Include all new nodes
result = g.update(new_edges=[(100, 200)], new_nodes=[100, 200])  # ✓
```

### StreamingGraph API Reference

#### Constructor
```python
g = ikc.StreamingGraph(graph_file, num_threads=None, verbose=False)
```

#### Methods

**`ikc(min_k=0, verbose=False, progress_bar=False) -> ClusterResult`**
- Run initial IKC clustering and initialize streaming state
- Must be called before any update operations

**`add_edges(edges, verbose=False) -> ClusterResult`**
- Add edges and update clustering incrementally
- `edges`: List of `(node_id, node_id)` tuples
- Returns updated clustering

**`add_nodes(nodes, verbose=False) -> ClusterResult`**
- Add isolated nodes to the graph
- `nodes`: List of node IDs
- Returns updated clustering

**`update(new_edges=None, new_nodes=None, verbose=False) -> ClusterResult`**
- Add both edges and nodes in a single operation
- More efficient than separate calls
- **Important**: All nodes referenced in `new_edges` must be included in `new_nodes` (if they don't already exist in the graph)
- Raises `ValueError` if an edge references a non-existent node not in `new_nodes`

**`begin_batch()`**
- Enter batch mode - accumulate updates without recomputation

**`commit_batch(verbose=False) -> ClusterResult`**
- Apply all pending updates and exit batch mode

#### Properties

**`current_clustering -> ClusterResult`**
- Get current clustering without triggering recomputation

**`last_update_stats -> dict`**
- Statistics from the last update operation:
  - `affected_nodes`: Nodes with changed core numbers
  - `invalidated_clusters`: Clusters that needed recomputation
  - `valid_clusters`: Clusters that remained valid
  - `merge_candidates`: Nodes involved in potential merges
  - `recompute_time_ms`: Time spent in localized recomputation
  - `total_time_ms`: Total update time

**`num_nodes -> int`**, **`num_edges -> int`**, **`max_core -> int`**, **`is_batch_mode -> bool`**

### Streaming Example

```python
import ikc

# Initialize with existing graph
g = ikc.StreamingGraph('network.tsv')
result = g.ikc(min_k=10, progress_bar=True)
print(f"Initial: {result.num_clusters} clusters")

# Incremental update
result = g.add_edges([(100, 200), (101, 202)])
stats = g.last_update_stats
print(f"Update: {stats['affected_nodes']} nodes affected in {stats['total_time_ms']:.2f}ms")

# Batch mode for multiple updates
g.begin_batch()
g.add_edges([(1, 2), (3, 4)])
g.add_nodes([500, 501])
result = g.commit_batch()
print(f"Batch complete: {result.num_clusters} clusters")
```

See `examples/streaming_example.py` for a complete demonstration.

### Performance Characteristics

**Expected Case (Sparse Updates)**
- Per-edge time: O(Δ² · log n) where Δ = max degree
- Most clusters remain valid
- Speedup: 10-100x vs full recomputation

**Worst Case (Dense Core Updates)**
- Per-edge time: O(n + m) (same as full recomputation)
- Occurs when adding edges to maximum k-core

**Batch Mode Benefits**
- Amortizes overhead across multiple updates
- Single core number recomputation for all edges
- Recommended for bulk updates

### When to Use Streaming vs Batch IKC

**Use Streaming IKC When:**
- Graph evolves incrementally over time
- Need up-to-date clustering after each update
- Updates are sparse (few edges at a time)
- Want to track which clusters changed

**Use Batch IKC (`Graph.ikc()`) When:**
- Have complete graph from the start
- Don't need intermediate clustering results
- Graph structure is static

### Limitations

- **Edge deletions not supported** - Only additions are handled
- **No state persistence** - Cannot save/load streaming state
- Cluster splits never occur with edge additions (proven mathematically)

### References

- Eleanor Wedell, Minhyuk Park, Dmitriy Korobskiy, Tandy Warnow, George Chacko; Center–periphery structure in research communities. *Quantitative Science Studies 2022*; 3 (1): 289–314.
- Sariyüce, A. E., Gedik, B., Jacques-Silva, G., Wu, K. L., & Çatalyürek, Ü. V. (2013).
  "Streaming algorithms for k-core decomposition."
  *Proceedings of the VLDB Endowment*, 6(6), 433-444.
