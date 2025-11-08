#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/functional.h>
#include "../lib/io/graph_io.h"
#include "../lib/algorithms/ikc.h"
#include "../lib/algorithms/streaming_ikc.h"
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
          py::arg("progress_callback") = nullptr,
          "Run Iterative K-Core Clustering algorithm");

    // Bind UpdateStats class
    py::class_<UpdateStats>(m, "UpdateStats")
        .def(py::init<>())
        .def_readonly("affected_nodes", &UpdateStats::affected_nodes)
        .def_readonly("invalidated_clusters", &UpdateStats::invalidated_clusters)
        .def_readonly("valid_clusters", &UpdateStats::valid_clusters)
        .def_readonly("merge_candidates", &UpdateStats::merge_candidates)
        .def_readonly("recompute_time_ms", &UpdateStats::recompute_time_ms)
        .def_readonly("total_time_ms", &UpdateStats::total_time_ms)
        .def("__repr__", [](const UpdateStats& s) {
            return "<UpdateStats affected_nodes=" + std::to_string(s.affected_nodes) +
                   " invalidated_clusters=" + std::to_string(s.invalidated_clusters) +
                   " valid_clusters=" + std::to_string(s.valid_clusters) +
                   " recompute_time_ms=" + std::to_string(s.recompute_time_ms) + ">";
        });

    // Bind StreamingIKC class
    py::class_<StreamingIKC>(m, "StreamingIKC")
        .def(py::init<const Graph&, uint32_t>(),
             py::arg("graph"),
             py::arg("min_k") = 0,
             "Initialize streaming IKC with a graph")
        .def("initial_clustering",
             &StreamingIKC::initial_clustering,
             py::arg("verbose") = false,
             py::arg("progress_callback") = nullptr,
             "Run initial IKC clustering")
        .def("add_edges",
             &StreamingIKC::add_edges,
             py::arg("edges"),
             py::arg("recompute") = true,
             py::arg("verbose") = false,
             "Add edges and update clustering incrementally")
        .def("add_nodes",
             &StreamingIKC::add_nodes,
             py::arg("nodes"),
             py::arg("recompute") = true,
             py::arg("verbose") = false,
             "Add isolated nodes to the graph")
        .def("update",
             &StreamingIKC::update,
             py::arg("edges"),
             py::arg("nodes"),
             py::arg("verbose") = false,
             "Add both edges and nodes in a single update")
        .def("begin_batch",
             &StreamingIKC::begin_batch,
             "Enter batch mode - accumulate updates without recomputation")
        .def("commit_batch",
             &StreamingIKC::commit_batch,
             py::arg("verbose") = false,
             "Exit batch mode and apply all pending updates")
        .def("get_clusters",
             &StreamingIKC::get_clusters,
             "Get current clustering")
        .def("get_core_numbers",
             &StreamingIKC::get_core_numbers,
             "Get current core numbers")
        .def("get_graph",
             &StreamingIKC::get_graph,
             "Get current graph")
        .def("get_last_stats",
             &StreamingIKC::get_last_stats,
             "Get statistics from last update")
        .def("get_num_nodes",
             &StreamingIKC::get_num_nodes,
             "Get number of nodes")
        .def("get_num_edges",
             &StreamingIKC::get_num_edges,
             "Get number of edges")
        .def("get_max_core",
             &StreamingIKC::get_max_core,
             "Get maximum core number")
        .def("is_batch_mode",
             &StreamingIKC::is_batch_mode,
             "Check if in batch mode")
        .def("__repr__", [](const StreamingIKC& s) {
            return "<StreamingIKC nodes=" + std::to_string(s.get_num_nodes()) +
                   " edges=" + std::to_string(s.get_num_edges()) +
                   " clusters=" + std::to_string(s.get_clusters().size()) +
                   " max_core=" + std::to_string(s.get_max_core()) + ">";
        });
}
