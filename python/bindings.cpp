#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "../lib/io/graph_io.h"
#include "../lib/algorithms/ikc.h"
#include "../lib/data_structures/graph.h"

namespace py = pybind11;

PYBIND11_MODULE(_ikc, m) {
    m.doc() = "Python bindings for IKC (Iterative K-Core Clustering)";

    // Bind Graph class
    py::class_<Graph>(m, "Graph")
        .def(py::init<>())
        .def_readonly("num_nodes", &Graph::num_nodes)
        .def_readonly("num_edges", &Graph::num_edges)
        .def_readonly("id_map", &Graph::id_map)
        .def("__repr__", [](const Graph& g) {
            return "<Graph nodes=" + std::to_string(g.num_nodes) +
                   " edges=" + std::to_string(g.num_edges) + ">";
        });

    // Bind Cluster class
    py::class_<Cluster>(m, "Cluster")
        .def(py::init<const std::vector<uint64_t>&, uint32_t, double>())
        .def_readonly("nodes", &Cluster::nodes)
        .def_readonly("k_value", &Cluster::k_value)
        .def_readonly("modularity", &Cluster::modularity)
        .def("__repr__", [](const Cluster& c) {
            return "<Cluster nodes=" + std::to_string(c.nodes.size()) +
                   " k=" + std::to_string(c.k_value) +
                   " modularity=" + std::to_string(c.modularity) + ">";
        });

    // Bind graph loading function
    m.def("load_graph",
          &load_undirected_tsv_edgelist_parallel,
          py::arg("filename"),
          py::arg("num_threads") = std::thread::hardware_concurrency(),
          py::arg("verbose") = false,
          "Load an undirected graph from TSV edge list file");

    // Bind IKC algorithm
    m.def("run_ikc",
          &iterative_kcore_decomposition,
          py::arg("graph"),
          py::arg("min_k") = 0,
          py::arg("orig_graph"),
          py::arg("verbose") = false,
          "Run Iterative K-Core Clustering algorithm");
}
