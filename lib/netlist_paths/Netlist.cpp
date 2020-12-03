#include <regex>
#include <boost/format.hpp>
#include "netlist_paths/Netlist.hpp"

using namespace netlist_paths;

/// Return a list of IDs of named vertices, optionally filter by regex.
std::vector<VertexID>
Netlist::getNamedVertexIds(const std::string &regex) const {
  std::vector<VertexID> vertices;
  std::regex nameRegex(regex);
  for (auto vertexId : netlist.getAllVertices()) {
    if (netlist.getVertex(vertexId).isNamed() &&
        (regex.empty() ||
         std::regex_search(netlist.getVertex(vertexId).getName(), nameRegex))) {
      vertices.push_back(vertexId);
    }
  }
  return vertices;
}

/// Return a sorted list of unique named entities in the netlist for searching.
std::vector<std::reference_wrapper<const Vertex>>
Netlist::getNamedVertices(const std::string &regex) const {
  // Collect vertices.
  std::vector<std::reference_wrapper<const Vertex>> vertices;
  for (auto vertexId : getNamedVertexIds(regex)) {
    vertices.push_back(std::ref(netlist.getVertex(vertexId)));
  }
  // Sort them.
  auto compare = [](const Vertex& a, const Vertex& b) {
                   return a.compareLessThan(b); };
  std::sort(vertices.begin(), vertices.end(), compare);
  return vertices;
}

VertexIDVec Netlist::readWaypoints(Waypoints waypoints) const {
  VertexIDVec waypointIDs;
  for (auto it = waypoints.getWaypoints().begin();
            it != waypoints.getWaypoints().end(); ++it) {
    VertexID vertex;
    // Start
    if (it == waypoints.getWaypoints().begin()) {
      vertex = netlist.getStartVertex(*it);
      if (vertex == netlist.nullVertex()) {
        throw Exception(std::string("could not find start vertex "+*it));
      }
    // Finish
    } else if (it+1 == waypoints.getWaypoints().end()) {
      vertex = netlist.getEndVertex(*it);
      if (vertex == netlist.nullVertex()) {
        throw Exception(std::string("could not find end vertex "+*it));
      }
    // Mid
    } else {
      vertex = netlist.getMidVertex(*it);
      if (vertex == netlist.nullVertex()) {
        throw Exception(std::string("could not find through vertex "+*it));
      }
    }
    waypointIDs.push_back(vertex);
  }
  return waypointIDs;
}

VertexIDVec Netlist::readAvoidPoints(Waypoints waypoints) const {
  VertexIDVec avoidPointIDs;
  for (auto name : waypoints.getAvoidPoints()) {
    avoidPointIDs.push_back(netlist.getVertexDesc(name));
  }
  // Sort the IDs so they can be binary searched.
  std::sort(avoidPointIDs.begin(), avoidPointIDs.end());
  return avoidPointIDs;
}
