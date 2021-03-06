#include <regex>
#include <boost/format.hpp>
#include "netlist_paths/Netlist.hpp"
#include "netlist_paths/ReadVerilatorXML.hpp"

using namespace netlist_paths;

Netlist::Netlist(const std::string &filename) {
  Options::getInstance(); // Create singleton object.
  ReadVerilatorXML(graph, files, dtypes, filename);
  graph.markAliasRegisters();
  graph.splitRegVertices();
  graph.updateVarAliases();
}

std::vector<Vertex*>
Netlist::createVertexPtrVec(VertexIDVec vertices) const {
  auto result = std::vector<Vertex*>();
  for (auto vertexId : vertices) {
    result.push_back(graph.getVertexPtr(vertexId));
  }
  return result;
}

std::vector<std::vector<Vertex*> >
Netlist::createVertexPtrVecVec(std::vector<VertexIDVec> paths) const {
  auto result = std::vector<std::vector<Vertex*> >();
  for (auto vertexIdVec : paths) {
    result.push_back(createVertexPtrVec(vertexIdVec));
  }
  return result;
}

const std::string
Netlist::reportMultipleMatches(VertexIDVec vertices,
                               const std::string name,
                               const std::string patternType) const {
  std::stringstream msg;
  msg << "multiple vertices matching " << patternType << " pattern: " << name << "\n";
  for (auto vertexID : vertices) {
    auto vertex = graph.getVertex(vertexID);
    msg << boost::format("%s %s\n") % vertex.getName() % vertex.getAstTypeStr();
  }
  return msg.str();
}

VertexID Netlist::getVertex(const std::string &name,
                                 VertexNetlistType vertexType) const {
  auto vertices = graph.getVertices(name, vertexType);
  if (vertices.size() > 1) {
    throw Exception(reportMultipleMatches(vertices, name));
  }
  if (vertices.size() == 1) {
    return vertices.front();
  }
  return graph.nullVertex();
}

VertexID Netlist::getRegVertex(const std::string &name, bool matchAny) const {
  auto vertices = graph.getRegVertices(name);
  if (!matchAny) {
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name, "register"));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
  } else {
    if (vertices.size()) {
      return vertices.front();
    }
  }
  return graph.nullVertex();
}

VertexID Netlist::getRegAliasVertex(const std::string &name, bool matchAny) const {
  auto vertices = graph.getRegAliasVertices(name);
  if (!matchAny) {
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name, "register alias"));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
  } else {
    if (vertices.size()) {
      return vertices.front();
    }
  }
  return graph.nullVertex();
}

VertexID Netlist::getStartVertex(const std::string &name, bool matchAny) const {
  auto vertices = graph.getStartVertices(name);
  if (!matchAny) {
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name, "begin point"));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
  } else {
    if (vertices.size()) {
      return vertices.front();
    }
  }
  return graph.nullVertex();
}

VertexID Netlist::getEndVertex(const std::string &name, bool matchAny) const {
  auto vertices = graph.getEndVertices(name);
  if (!matchAny) {
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name, "end point"));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
  } else {
    if (vertices.size()) {
      return vertices.front();
    }
  }
  return graph.nullVertex();
}

VertexID Netlist::getMidVertex(const std::string &name, bool matchAny) const {
  auto vertices = graph.getMidVertices(name);
  if (!matchAny) {
    if (vertices.size() > 1) {
      throw Exception(reportMultipleMatches(vertices, name, "mid point"));
    }
    if (vertices.size() == 1) {
      return vertices.front();
    }
  } else {
    if (vertices.size()) {
      return vertices.front();
    }
  }
  return graph.nullVertex();
}

const std::string
Netlist::getVertexDTypeStr(const std::string &name,
                           VertexNetlistType vertexType) const {
  auto vertex = getVertex(name, vertexType);
  if (vertex == graph.nullVertex()) {
    throw Exception(std::string("could not find vertex "+name));
  }
  return graph.getVertex(vertex).getDTypeStr();
}

size_t
Netlist::getVertexDTypeWidth(const std::string &name,
                             VertexNetlistType vertexType) const {
  auto vertex = getVertex(name, vertexType);
  if (vertex == graph.nullVertex()) {
    throw Exception(std::string("could not find vertex "+name));
  }
  return graph.getVertex(vertex).getDTypeWidth();
}

const std::shared_ptr<DType> Netlist::getDType(const std::string &name) const {
  auto dtype = std::find_if(std::begin(dtypes), std::end(dtypes),
                            [&name](const std::shared_ptr<DType> &dt) {
                              return dt->getName() == name; });
  if (dtype != std::end(dtypes)) {
    return *dtype;
  } else {
    return std::shared_ptr<DType>();
  }
}

size_t Netlist::getDTypeWidth(const std::string &name) const {
  if (auto dtype = getDType(name)) {
    return dtype->getWidth();
  } else {
    throw Exception(std::string("could not find dtype "+name));
  }
}

VertexIDVec Netlist::readWaypoints(Waypoints waypoints) const {
  VertexIDVec waypointIDs;
  for (auto it = waypoints.getWaypoints().begin();
            it != waypoints.getWaypoints().end(); ++it) {
    VertexID vertex;
    // Start
    if (it == waypoints.getWaypoints().begin()) {
      vertex = getStartVertex(*it, Options::getInstance().isMatchAnyVertex());
      if (vertex == graph.nullVertex()) {
        throw Exception(std::string("could not find start vertex matching ")+*it);
      }
    // Finish
    } else if (it+1 == waypoints.getWaypoints().end()) {
      vertex = getEndVertex(*it, Options::getInstance().isMatchAnyVertex());
      if (vertex == graph.nullVertex()) {
        throw Exception(std::string("could not find end vertex matching ")+*it);
      }
    // Mid
    } else {
      vertex = getMidVertex(*it, Options::getInstance().isMatchAnyVertex());
      if (vertex == graph.nullVertex()) {
        throw Exception(std::string("could not find through vertex ")+*it);
      }
    }
    waypointIDs.push_back(vertex);
  }
  return waypointIDs;
}

VertexIDVec Netlist::readAvoidPoints(Waypoints waypoints) const {
  VertexIDVec avoidPointIDs;
  for (auto name : waypoints.getAvoidPoints()) {
    auto vertex = getMidVertex(name, Options::getInstance().isMatchAnyVertex());
    if (vertex == graph.nullVertex()) {
      throw Exception(std::string("could not find vertex to avoid ")+name);
    }
    avoidPointIDs.push_back(vertex);
  }
  // Sort the IDs so they can be binary searched.
  std::sort(avoidPointIDs.begin(), avoidPointIDs.end());
  return avoidPointIDs;
}

bool Netlist::startpointExists(const std::string &name) const {
  return getStartVertex(name, false) != graph.nullVertex();
}

bool Netlist::endpointExists(const std::string &name) const {
  return getEndVertex(name, false) != graph.nullVertex();
}

bool Netlist::anyStartpointExists(const std::string &name) const {
  return getStartVertex(name, true) != graph.nullVertex();
}

bool Netlist::anyEndpointExists(const std::string &name) const {
  return getEndVertex(name, true) != graph.nullVertex();
}

bool Netlist::anyRegExists(const std::string &name) const {
  // Allow matching with register or register alias variables.
  auto regVertices = graph.getRegVertices(name);
  auto regAliasVertices = graph.getRegAliasVertices(name);
  return (regVertices.size() != 0) || (regAliasVertices.size() != 0);
}

bool Netlist::regExists(const std::string &name) const {
  // Allow matching with register or register alias variables.
  return (getRegVertex(name, false) != graph.nullVertex()) ||
         (getRegAliasVertex(name, false) != graph.nullVertex());
}

bool Netlist::pathExists(Waypoints waypoints) const {
  auto waypointIDs = readWaypoints(waypoints);
  auto avoidPointIDs = readAvoidPoints(waypoints);
  return !graph.getAnyPointToPoint(waypointIDs, avoidPointIDs).empty();
}

std::vector<Vertex*> Netlist::getAnyPath(Waypoints waypoints) const {
  auto waypointIDs = readWaypoints(waypoints);
  auto avoidPointIDs = readAvoidPoints(waypoints);
  return createVertexPtrVec(graph.getAnyPointToPoint(waypointIDs,
                                                       avoidPointIDs));
}

std::vector<std::vector<Vertex*> > Netlist::getAllPaths(Waypoints waypoints) const {
  auto waypointIDs = readWaypoints(waypoints);
  auto avoidPointIDs = readAvoidPoints(waypoints);
  return createVertexPtrVecVec(graph.getAllPointToPoint(waypointIDs,
                                                          avoidPointIDs));
}

std::vector<std::vector<Vertex*> > Netlist::getAllFanOut(const std::string startName) const {
  auto vertex = getStartVertex(startName, Options::getInstance().isMatchAnyVertex());
  if (vertex == graph.nullVertex()) {
    throw Exception(std::string("could not find start vertex "+startName));
  }
  return createVertexPtrVecVec(graph.getAllFanOut(vertex));
}

std::vector<std::vector<Vertex*> > Netlist::getAllFanIn(const std::string endName) const {
  auto vertex = getEndVertex(endName, Options::getInstance().isMatchAnyVertex());
  if (vertex == graph.nullVertex()) {
    throw Exception(std::string("could not find end vertex "+endName));
  }
  return createVertexPtrVecVec(graph.getAllFanIn(vertex));
}

std::vector<std::reference_wrapper<const Vertex> >
Netlist::getNamedVertices(const std::string pattern) const {
  // Collect vertices.
  std::vector<std::reference_wrapper<const Vertex>> vertices;
  for (auto vertexId : graph.getVertices(pattern, VertexNetlistType::IS_NAMED)) {
    vertices.push_back(std::ref(graph.getVertex(vertexId)));
  }
  // Sort them.
  auto compare = [](const Vertex& a, const Vertex& b) {
                   return a.compareLessThan(b); };
  std::sort(vertices.begin(), vertices.end(), compare);
  return vertices;
}
