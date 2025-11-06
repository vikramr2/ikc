# IKC - Iterative K-Core Clustering

Fast C++ implementation of Iterative K-Core Clustering with Python wrapper.

## Project Structure

```
ikc/
├── app/          # C++ application
├── lib/          # C++ libraries
├── python/       # Python wrapper
│   ├── ikc/      # Python package
│   └── example.py
├── data/         # Test datasets
└── build/        # Build directory
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

### Installation

1. **Build the C++ executable first** (see above)

2. **Install the Python package:**

```bash
pip install -e .
```

Or without installation:
```python
import sys
sys.path.insert(0, 'path/to/ikc/python')
import ikc
```

3. **Verify installation:**

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

# Access the underlying pandas DataFrame
print(c.data.head())
```

### Python API Reference

#### `ikc.load_graph(graph_file)`

Load a graph from a TSV edge list file.

**Parameters:**
- `graph_file` (str): Path to the graph edge list file (TSV format)

**Returns:**
- `Graph`: Graph object ready for clustering

#### `Graph.ikc(min_k=0, num_threads=None, quiet=False)`

Run the Iterative K-Core Clustering algorithm.

**Parameters:**
- `min_k` (int): Minimum k value for valid clusters (default: 0)
- `num_threads` (int, optional): Number of threads to use (default: hardware concurrency)
- `quiet` (bool): If True, suppress verbose output from the C++ program (default: False)

**Returns:**
- `ClusterResult`: Object containing the clustering results

#### `ClusterResult.save(filename, tsv=False)`

Save clustering results to a file.

**Parameters:**
- `filename` (str): Output file path
- `tsv` (bool): If True, save as TSV with only node_id and cluster_id (no header). If False, save as CSV with all columns (default: False)

#### `ClusterResult` Properties

- `num_clusters`: Number of clusters found
- `num_nodes`: Number of nodes in the clustering
- `data`: Underlying pandas DataFrame with columns: `node_id`, `cluster_id`, `k_value`, `modularity`

### Python Example

```python
import ikc

# Load graph
g = ikc.load_graph('data/cit_hepph.tsv')

# Run IKC with min_k=10, using 4 threads
clusters = g.ikc(min_k=10, num_threads=4)

# Print summary
print(clusters)  # ClusterResult(nodes=34546, clusters=156)

# Save in TSV format
clusters.save('output.tsv', tsv=True)

# Save in CSV format with all columns
clusters.save('output.csv', tsv=False)

# Access the data
print(clusters.data.head(10))

# Get cluster statistics
print(f"Total clusters: {clusters.num_clusters}")
print(f"Total nodes: {clusters.num_nodes}")
```

### Run Example Script

```bash
python3 python/example.py
```

## Requirements

### C++
- CMake 3.10+
- C++17 compatible compiler
- pthread support

### Python
- Python 3.7+
- pandas > 1.0.0

## Input Format

The input graph file should be a TSV (tab-separated) edge list with two columns:
```
node1	node2
node3	node4
...
```

No header required. Nodes can be any integer IDs.
