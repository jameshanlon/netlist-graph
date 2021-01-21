#ifndef NETLIST_PATHS_NETLIST_HPP
#define NETLIST_PATHS_NETLIST_HPP

#include <memory>
#include <iostream>
#include <ostream>
#include <sstream>
#include <boost/format.hpp>
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

  //===--------------------------------------------------------------------===//
  // Utility functions.
  //===--------------------------------------------------------------------===//

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

  //===--------------------------------------------------------------------===//
  // Vertex lookup, with error reporting.
  //===--------------------------------------------------------------------===//

  const std::string reportMultipleMatches(VertexIDVec vertices,
                                          const std::string name,
                                          const std::string patternType="") const {
    std::stringstream msg;
    msg << "multiple vertices matching " << patternType << " pattern: " << name << "\n";
    for (auto vertexID : vertices) {
      auto vertex = netlist.getVertex(vertexID);
      msg << boost::format("%s %s\n") % vertex.getName() % vertex.getAstTypeStr();
    }
    return msg.str();
  }

  VertexID getVertexRegex(const std::string &name,
                          VertexGraphType vertexType=VertexGraphType::ANY) const {
    auto vertices = netlist.getVertices(name, vertexType);
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
    return netlist.nullVertex();
  }

  VertexID getRegVertex(const std::string &name) const {
    auto vertices = netlist.getRegVertices(name);
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name, "register"));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
    return netlist.nullVertex();
  }

  VertexID getBeginVertex(const std::string &name) const {
    auto vertices = netlist.getStartVertices(name);
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name, "begin point"));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
    return netlist.nullVertex();
  }

  VertexID getEndVertex(const std::string &name) const {
    auto vertices = netlist.getEndVertices(name);
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name, "end point"));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
    return netlist.nullVertex();
  }

  VertexID getMidVertex(const std::string &name) const {
    auto vertices = netlist.getMidVertices(name);
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name, "mid point"));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
    return netlist.nullVertex();
  }

public:
  Netlist() = delete;
  Netlist(const std::string &filename) {
    ReadVerilatorXML(netlist, files, dtypes, filename);
    netlist.propagateRegisters();
    netlist.splitRegVertices();
    netlist.checkGraph();
  }

  //===--------------------------------------------------------------------===//
  // Reporting of names and types.
  //===--------------------------------------------------------------------===//

  const std::string getVertexDTypeStr(const std::string &name,
                                      VertexGraphType vertexType=VertexGraphType::ANY) const {
    auto vertex = getVertexRegex(name, vertexType);
    if (vertex == netlist.nullVertex()) {
      throw Exception(std::string("could not find vertex "+name));
    }
    return netlist.getVertex(vertex).getDTypeStr();
  }

  size_t getVertexDTypeWidth(const std::string &name,
                             VertexGraphType vertexType=VertexGraphType::ANY) const {
    auto vertex = getVertexRegex(name, vertexType);
    if (vertex == netlist.nullVertex()) {
      throw Exception(std::string("could not find vertex "+name));
    }
    return netlist.getVertex(vertex).getDTypeWidth();
  }

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
    return getBeginVertex(name) != netlist.nullVertex();
  }

  bool endpointExists(const std::string &name) const {
    return getEndVertex(name) != netlist.nullVertex();
  }

  bool anyStartpointExists(const std::string &name) const {
    auto vertices = netlist.getStartVertices(name);
    return vertices.size() != 0;
  }

  bool anyEndpointExists(const std::string &name) const {
    auto vertices = netlist.getEndVertices(name);
    return vertices.size() != 0;
  }

  bool anyRegExists(const std::string &name) const {
    auto vertices = netlist.getRegVertices(name);
    return vertices.size() != 0;
  }

  bool regExists(const std::string &name) const {
    return getRegVertex(name) != netlist.nullVertex();
  }

  /// Return a Boolean to indicate whether any path exists between two points.
  bool pathExists(Waypoints waypoints) {
    auto waypointIDs = readWaypoints(waypoints);
    auto avoidPointIDs = readAvoidPoints(waypoints);
    return !netlist.getAnyPointToPoint(waypointIDs, avoidPointIDs).empty();
  }

  /// Return any path between two points.
  std::vector<Vertex*> getAnyPath(Waypoints waypoints) const {
    auto waypointIDs = readWaypoints(waypoints);
    auto avoidPointIDs = readAvoidPoints(waypoints);
    return createVertexPtrVec(netlist.getAnyPointToPoint(waypointIDs,
                                                         avoidPointIDs));
  }

  /// Return all paths between two points, useful for testing.
  std::vector<std::vector<Vertex*> > getAllPaths(Waypoints waypoints) const {
    auto waypointIDs = readWaypoints(waypoints);
    auto avoidPointIDs = readAvoidPoints(waypoints);
    return createVertexPtrVecVec(netlist.getAllPointToPoint(waypointIDs,
                                                            avoidPointIDs));
  }

  /// Return a vector of paths fanning out from a particular start point.
  std::vector<std::vector<Vertex*> > getAllFanOut(const std::string startName) const {
    auto vertex = getBeginVertex(startName);
    if (vertex == netlist.nullVertex()) {
      throw Exception(std::string("could not find begin vertex "+startName));
    }
    return createVertexPtrVecVec(netlist.getAllFanOut(vertex));
  }

  /// Return a vector of paths fanning out from a particular start point.
  std::vector<std::vector<Vertex*> > getAllFanIn(const std::string endName) const {
    auto vertex = getEndVertex(endName);
    if (vertex == netlist.nullVertex()) {
      throw Exception(std::string("could not find end vertex "+endName));
    }
    return createVertexPtrVecVec(netlist.getAllFanIn(vertex));
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

  bool isEmpty() const { return netlist.numVertices() == 0; }
};

}; // End namespace.

#endif // NETLIST_PATHS_NETLIST_HPP
