#ifndef NETLIST_PATHS_NETLIST_PATHS_HPP
#define NETLIST_PATHS_NETLIST_PATHS_HPP

#include <memory>
#include <iostream>
#include <ostream>
#include "netlist_paths/Netlist.hpp"
#include "netlist_paths/ReadVerilatorXML.hpp"

namespace netlist_paths {

/// Wrapper for Python to manage the netlist paths object.
class NetlistPaths {
  Netlist netlist;
  std::vector<File> files;
  std::vector<std::shared_ptr<DType>> dtypes;
  std::vector<VertexDesc> waypoints;
  std::vector<VertexDesc> getNamedVertexIds(const std::string& regex="") const;

public:
  NetlistPaths() = delete;
  NetlistPaths(const std::string &filename) {
    ReadVerilatorXML(netlist, files, dtypes, filename);
    netlist.splitRegVertices();
    netlist.checkGraph();
  }
  //===--------------------------------------------------------------------===//
  // Waypoints.
  //===--------------------------------------------------------------------===//
  void addStartpoint(const std::string &name) {
    waypoints.push_back(netlist.getStartVertex(name));
  }
  void addEndpoint(const std::string &name) {
    waypoints.push_back(netlist.getEndVertex(name));
  }
  void addWaypoint(const std::string &name) {
    waypoints.push_back(netlist.getMidVertex(name));
  }
  std::size_t numWaypoints() const { return waypoints.size(); }
  void clearWaypoints() { waypoints.clear(); }
  //===--------------------------------------------------------------------===//
  // Reporting of names and types.
  //===--------------------------------------------------------------------===//
  const std::string getDTypeStr(const std::string &name,
                                VertexGraphType vertexType=VertexGraphType::ANY) const {
    auto vertex = netlist.getVertexDescRegex(name, vertexType);
    if (vertex != netlist.nullVertex()) {
      return netlist.getVertex(vertex).getDTypeString();
    } else {
      return std::string("none");
    }
  }
  void dumpNames(std::ostream &os, const std::string &regex) const;
  void dumpNamesStdOut(const std::string &regex) const { dumpNames(std::cout, regex); }
  //void printPathReport(const Path &path) const;
  //void printPathReport(const std::vector<Path> &paths) const;
  //===--------------------------------------------------------------------===//
  // Basic path querying.
  //===--------------------------------------------------------------------===//
  bool startpointExists(const std::string &name) const noexcept {
    return netlist.getStartVertex(name) != netlist.nullVertex();
  }
  bool endpointExists(const std::string &name) const noexcept {
    return netlist.getEndVertex(name) != netlist.nullVertex();
  }
  bool regExists(const std::string &name) const noexcept {
    return netlist.getRegVertex(name) != netlist.nullVertex();
  }
  bool pathExists(const std::string &start, const std::string &end) {
    clearWaypoints();
    // Check that the start and end points exist.
    auto startPoint = netlist.getStartVertex(start);
    auto endPoint = netlist.getEndVertex(end);
    if (startPoint == netlist.nullVertex() ||
        endPoint == netlist.nullVertex()) {
      return false;
    }
    // Check the path exists.
    waypoints.push_back(startPoint);
    waypoints.push_back(endPoint);
    return !netlist.getAnyPointToPoint(waypoints).empty();
  }
  //===--------------------------------------------------------------------===//
  // Netlist access.
  //===--------------------------------------------------------------------===//
  void dumpDotFile(const std::string &outputFilename) const {
    netlist.dumpDotFile(outputFilename);
  }
  std::vector<std::reference_wrapper<const Vertex>>
  getNamedVertices(const std::string &regex="") const;
};

}; // End namespace.

#endif // NETLIST_PATHS_NETLIST_PATHS_HPP