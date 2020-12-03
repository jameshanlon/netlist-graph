#ifndef NETLIST_PATHS_NETLIST_HPP
#define NETLIST_PATHS_NETLIST_HPP

#include <memory>
#include <iostream>
#include <ostream>
#include "netlist_paths/Exception.hpp"
#include "netlist_paths/Graph.hpp"
#include "netlist_paths/Options.hpp"
#include "netlist_paths/ReadVerilatorXML.hpp"
#include "netlist_paths/Waypoints.hpp"

namespace netlist_paths {

/// Wrapper for Python to manage the netlist paths object.
class Netlist {
  Graph netlist;
  std::vector<File> files;
  std::vector<std::shared_ptr<DType>> dtypes;
  std::vector<VertexID> waypoints;

  /// Get a DType by name.
  const std::shared_ptr<DType> getDType(const std::string &name) const {
    auto dtype = std::find_if(std::begin(dtypes), std::end(dtypes),
                              [&name](const std::shared_ptr<DType> &dt) {
                                return dt->getName() == name; });
    if (dtype != std::end(dtypes)) {
      return *dtype;
    } else {
      return std::shared_ptr<DType>();
    }
  }

  std::vector<VertexID> getNamedVertexIds(const std::string &regex="") const;

  std::vector<Vertex*> createVertexPtrVec(VertexIDVec vertices) const {
    auto result = std::vector<Vertex*>();
    for (auto vertexId : vertices) {
      result.push_back(netlist.getVertexPtr(vertexId));
    }
    return result;
  }

  std::vector<std::vector<Vertex*> >
  createVertexPtrVecVec(std::vector<VertexIDVec> paths) const {
    auto result = std::vector<std::vector<Vertex*> >();
    for (auto vertexIdVec : paths) {
      result.push_back(createVertexPtrVec(vertexIdVec));
    }
    return result;
  }

public:
  Netlist() = delete;
  Netlist(const std::string &filename) {
    ReadVerilatorXML(netlist, files, dtypes, filename);
    netlist.splitRegVertices();
    netlist.checkGraph();
  }

  //===--------------------------------------------------------------------===//
  // Reporting of names and types.
  //===--------------------------------------------------------------------===//

  const std::string getVertexDTypeStr(const std::string &name,
                                      VertexGraphType vertexType=VertexGraphType::ANY) const {
    auto vertex = netlist.getVertexDescRegex(name, vertexType);
    if (vertex != netlist.nullVertex()) {
      return netlist.getVertex(vertex).getDTypeStr();
    } else {
      throw Exception(std::string("could not find vertex "+name));
    }
  }

  size_t getVertexDTypeWidth(const std::string &name,
                             VertexGraphType vertexType=VertexGraphType::ANY) const {
    auto vertex = netlist.getVertexDescRegex(name, vertexType);
    if (vertex != netlist.nullVertex()) {
      return netlist.getVertex(vertex).getDTypeWidth();
    } else {
      throw Exception(std::string("could not find vertex "+name));
    }
  }

  size_t getDTypeWidth(const std::string &name) const {
    if (auto dtype = getDType(name)) {
      return dtype->getWidth();
    } else {
      throw Exception(std::string("could not find dtype "+name));
    }
  }

  //===--------------------------------------------------------------------===//
  // Waypoints.
  //===--------------------------------------------------------------------===//

  VertexIDVec readWaypoints(Waypoints waypoints) const;
  VertexIDVec readAvoidPoints(Waypoints waypoints) const;

  //===--------------------------------------------------------------------===//
  // Basic path querying.
  //===--------------------------------------------------------------------===//

  bool startpointExists(const std::string &name) const {
    return netlist.getStartVertex(name) != netlist.nullVertex();
  }

  bool endpointExists(const std::string &name) const {
    return netlist.getEndVertex(name) != netlist.nullVertex();
  }

  bool regExists(const std::string &name) const {
    return netlist.getRegVertex(name) != netlist.nullVertex();
  }

  /// Return a Boolean to indicate whether any path exists between two points.
  bool pathExists(Waypoints waypoints) {
    auto waypointIDs = readWaypoints(waypoints);
    auto avoidPointIDs = readAvoidPoints(waypoints);
    return !netlist.getAnyPointToPoint(waypointIDs, avoidPointIDs).empty();
  }

  /// Return any path between two points.
  std::vector<Vertex*> getAnyPath(Waypoints waypoints) {
    auto waypointIDs = readWaypoints(waypoints);
    auto avoidPointIDs = readAvoidPoints(waypoints);
    return createVertexPtrVec(netlist.getAnyPointToPoint(waypointIDs, avoidPointIDs));
  }

  /// Return any path between two points.
  std::vector<std::vector<Vertex*> > getAllPaths(Waypoints waypoints) {
    auto waypointIDs = readWaypoints(waypoints);
    return createVertexPtrVecVec(netlist.getAllPointToPoint(waypointIDs));
  }

  /// Return a vector of paths fanning out from a particular start point.
  std::vector<std::vector<Vertex*> > getAllFanOut(const std::string startName) const {
    auto startVertex = netlist.getStartVertex(startName);
    if (startVertex == netlist.nullVertex()) {
      throw Exception(std::string("could not find start vertex "+startName));
    }
    return createVertexPtrVecVec(netlist.getAllFanOut(startVertex));
  }

  /// Return a vector of paths fanning out from a particular start point.
  std::vector<std::vector<Vertex*> > getAllFanIn(const std::string finishName) const {
    auto finishVertex = netlist.getEndVertex(finishName);
    if (finishVertex == netlist.nullVertex()) {
      throw Exception(std::string("could not find finish vertex "+finishName));
    }
    return createVertexPtrVecVec(netlist.getAllFanIn(finishVertex));
  }

  //===--------------------------------------------------------------------===//
  // Netlist access.
  //===--------------------------------------------------------------------===//

  std::vector<std::reference_wrapper<const Vertex>>
  getNamedVertices(const std::string &regex="") const;

  std::vector<Vertex*> getNamedVerticesPtr(const std::string &regex="") const {
    return createVertexPtrVec(getNamedVertexIds(regex));
  }

  void dumpDotFile(const std::string &outputFilename) const {
    netlist.dumpDotFile(outputFilename);
  }
};

}; // End namespace.

#endif // NETLIST_PATHS_NETLIST_HPP
