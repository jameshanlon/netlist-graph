#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <regex>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include "netlist_paths/Debug.hpp"
#include "netlist_paths/Exception.hpp"
#include "netlist_paths/Graph.hpp"
#include "netlist_paths/Options.hpp"

using namespace netlist_paths;

class DfsVisitor : public boost::default_dfs_visitor {
private:
  ParentMap &parentMap;
  bool allPaths;
public:
  DfsVisitor(ParentMap &parentMap, bool allPaths) :
      parentMap(parentMap), allPaths(allPaths) {}
  // Visit only the edges of the DFS graph.
  template<typename Edge, typename Graph>
  void tree_edge(Edge edge, const Graph &graph) const {
    if (!allPaths) {
      VertexID src, dst;
      src = boost::source(edge, graph);
      dst = boost::target(edge, graph);
      parentMap[dst].push_back(src);
    }
    return;
  }
  // Visit all edges of a vertex.
  template<typename Edge, typename Graph>
  void examine_edge(Edge edge, const Graph &graph) const {
    if (allPaths) {
      VertexID src, dst;
      src = boost::source(edge, graph);
      dst = boost::target(edge, graph);
      parentMap[dst].push_back(src);
    }
    return;
  }
};

/// Register vertices are split into 'destination' registers only with in edges
/// and 'source' registers only with out edges. This implies graph connectivity
/// follows combinatorial paths in the netlist and allows traversals of the
/// graph to trace combinatorial timing paths.
void Graph::splitRegVertices() {
  BGL_FORALL_VERTICES(v, graph, InternalGraph) {
    if (graph[v].isReg()) {
      // Collect all adjacent vertices.
      std::vector<VertexID> adjacentVertices;
      BGL_FORALL_ADJ(v, adjVertex, graph, InternalGraph) {
        adjacentVertices.push_back(adjVertex);
      }
      // Create a new 'source' reg vertex.
      Vertex srcReg(graph[v]);
      srcReg.setSrcReg();
      auto srcRegVertex = boost::add_vertex(srcReg, graph);
      // Move the out edges to the src reg (while not iterating).
      for (auto adjVertex : adjacentVertices) {
        boost::remove_edge(v, adjVertex, graph);
        boost::add_edge(srcRegVertex, adjVertex, graph);
      }
    }
  }
}

///// Remove duplicate vertices from the graph by sorting them comparing each
///// vertex to its neighbours.
//void Netlist::mergeDuplicateVertices() {
//  std::vector<VertexDesc> vs;
//  BGL_FORALL_VERTICES(v, graph, Graph) {
//    if (!graph[v].isLogic()) {
//      vs.push_back(v);
//    }
//  }
//  auto compare = [this](const VertexDesc a, const VertexDesc b) {
//                   return graph[a].compareLessThan(graph[b]); };
//  std::sort(std::begin(vs), std::end(vs), compare);
//  VertexDesc current = vs[0];
//  unsigned count = 0;
//  for (size_t i=1; i<vs.size(); i++) {
//    if (graph[vs[i]].compareEqual(graph[current])) {
//      DEBUG(std::cout << "DUPLICATE VERTEX " << graph[vs[i]].name << "\n");
//      BGL_FORALL_ADJ(vs[i], v, graph, Graph) {
//        boost::add_edge(current, v, graph);
//        boost::remove_edge(vs[i], v, graph);
//      }
//      // We mark duplicate vertices as deleted since it is expensive to remove
//      // them from the graph as vertices are stored in a vecS. Using a listS
//      // is less performant.
//      graph[vs[i]].setDeleted();
//      ++count;
//    } else {
//      current = vs[i];
//    }
//  }
//  INFO(std::cout << "Removed " << count << " duplicate vertices\n");
//}

/// Perform some checks on the netlist and emit warnings if necessary.
void Graph::checkGraph() const {
  BGL_FORALL_VERTICES(v, graph, InternalGraph) {
    // Check there are no Vlvbound nodes.
    if (graph[v].getName().find("__Vlvbound") != std::string::npos) {
      std::cout << "Warning: " << graph[v].toString() << " vertex in netlist\n";
    }
    // Source registers don't have in edges.
    if (graph[v].isSrcReg()) {
      if (boost::in_degree(v, graph) > 0)
         std::cout << "Warning: source reg " << graph[v].toString()
                   << " has in edges" << "\n";
    }
    // Destination registers don't have out edges.
    if (graph[v].isDstReg()) {
      if (boost::out_degree(v, graph) > 0)
        std::cout << "Warning: destination reg " << graph[v].toString()
                  << " has out edges"<<"\n";
    }
    // NOTE: vertices may be incorrectly marked as reg if a field of a
    // structure has a delayed assignment to a field of it.
  }
}

/// Return a list of Vertex objects in the graph.
VertexIDVec Graph::getAllVertices() const {
  VertexIDVec vs;
  BGL_FORALL_VERTICES(v, graph, InternalGraph) {
    vs.push_back(v);
  }
  return vs;
}

/// Dump a Graphviz dotfile of the netlist graph for visualisation.
void Graph::dumpDotFile(const std::string &outputFilename) const {
  std::ofstream outputFile(outputFilename);
  if (!outputFile.is_open()) {
    throw Exception(std::string("unable to open ")+outputFilename);
  }
  // Loop over all vertices and print properties.
  outputFile << "digraph netlist {\n";
  BGL_FORALL_VERTICES(v, graph, InternalGraph) {
    outputFile << v << " ["
       << "label=\"" << graph[v].getName() << "\", "
       << "type=\"" << graph[v].getAstTypeString() << "\""
       << "]\n";
  }
  // Loop over all edges.
  BGL_FORALL_EDGES(e, graph, InternalGraph) {
    outputFile << boost::source(e, graph) << " -> "
               << boost::target(e, graph) << ";\n";
  }
  outputFile << "}\n";
  outputFile.close();
  // Print command line to generate graph file.
  INFO(std::cout << "dot -Tpdf " << outputFilename << " -o graph.pdf\n");
}

/// Lookup a vertex by name.
VertexID Graph::getVertexDesc(const std::string &name) const {
  BGL_FORALL_VERTICES(v, graph, InternalGraph) {
    if (graph[v].getName() == name) {
      return v;
    }
  }
  return nullVertex();
}

/// Lookup a vertex using a regex pattern and function specifying a type.
VertexID Graph::getVertexDescRegex(const std::string &name,
                                       VertexGraphType graphType) const {
  auto nameRegexStr(name);
  // Ignoring '/' (when supplying a heirarchical ref).
  std::replace(nameRegexStr.begin(), nameRegexStr.end(), '/', '.');
  // Or '_' (when supplying a flattened name).
  std::replace(nameRegexStr.begin(), nameRegexStr.end(), '_', '.');
  // Wildcard matching.
  if (Options::getInstance().getMatchWildcard()) {
    boost::replace_all(nameRegexStr, "?", ".");
    boost::replace_all(nameRegexStr, "*", ".*");
  }
  // Catch any errors in the regex string.
  std::regex nameRegex;
  try {
    nameRegex.assign(nameRegexStr);
  } catch(std::regex_error e) {
    throw Exception(std::string("malformed regular expression: ")+e.what());
  }
  // Search the vertices.
  // TODO: create a list of candidate vertices, rather than iterating all vertices.
  BGL_FORALL_VERTICES(v, graph, InternalGraph) {
    if (((graphType == VertexGraphType::ANY) ? true : graph[v].isGraphType(graphType)) &&
        std::regex_search(graph[v].getName(), nameRegex)) {
      return v;
    }
  }
  return nullVertex();
}

void Graph::dumpPath(const VertexIDVec &path) const {
  for (auto v : path) {
    if (!graph[v].isLogic()) {
      std::cout << "  " << graph[v].getName() << "\n";
    }
  }
}

/// Given the tree structure from a DFS, traverse the tree from leaf to root to
/// return a path.
VertexIDVec Graph::determinePath(ParentMap &parentMap,
                                 VertexIDVec path,
                                 VertexID startVertex,
                                 VertexID endVertex) const {
  path.push_back(endVertex);
  if (endVertex == startVertex) {
    return path;
  }
  if (parentMap[endVertex].size() == 0) {
    return VertexIDVec();
  }
  assert(parentMap[endVertex].size() == 1);
  auto nextVertex = parentMap[endVertex].front();
  assert(std::find(std::begin(path),
                   std::end(path),
                   nextVertex) == std::end(path));
  return determinePath(parentMap, path, startVertex, nextVertex);
}

/// Determine all paths between a start and an end point.
/// This performs a DFS starting at the end point. It is not feasible for large
/// graphs since the number of simple paths grows exponentially.
void Graph::determineAllPaths(ParentMap &parentMap,
                              std::vector<VertexIDVec> &result,
                              VertexIDVec path,
                              VertexID startVertex,
                              VertexID endVertex) const {
  path.push_back(endVertex);
  if (endVertex == startVertex) {
    INFO(std::cout << "FOUND PATH\n");
    result.push_back(path);
    return;
  }
  INFO(std::cout<<"length "<<path.size()
                <<" vertex "<<graph[endVertex].toString()<<"\n");
  INFO(dumpPath(path));
  INFO(std::cout<<(parentMap[endVertex].empty()?"DEAD END\n":""));
  for (auto vertex : parentMap[endVertex]) {
    if (std::find(std::begin(path), std::end(path), vertex) == std::end(path)) {
      determineAllPaths(parentMap, result, path, startVertex, vertex);
    } else {
      INFO(std::cout << "CYCLE DETECTED\n");
    }
  }
}

///// Report all paths fanning out from a net/register/port.
//std::vector<Path> Netlist::
//getAllFanOut(VertexDesc startVertex) const {
//  INFO(std::cout << "Performing DFS from "
//                 << graph[startVertex].name << "\n");
//  ParentMap parentMap;
//  boost::depth_first_search(graph,
//      boost::visitor(DfsVisitor(parentMap, false))
//        .root_vertex(startVertex));
//  // Check for a path between startPoint and each register.
//  std::vector<Path> paths;
//  BGL_FORALL_VERTICES(v, graph, Graph) {
//    if (isEndPoint(graph[v])) {
//      auto path = determinePath(parentMap,
//                                Path(),
//                                startVertex,
//                                static_cast<VertexDesc>(graph[v].id));
//      if (!path.empty()) {
//        std::reverse(std::begin(path), std::end(path));
//        paths.push_back(path);
//      }
//    }
//  }
//  return paths;
//}
//
///// Report all paths fanning into a net/register/port.
//std::vector<Path> Netlist::
//getAllFanIn(VertexDesc endVertex) const {
//  auto reverseGraph = boost::make_reverse_graph(graph);
//  INFO(std::cout << "Performing DFS in reverse graph from "
//                 << graph[endVertex].name << "\n");
//  ParentMap parentMap;
//  boost::depth_first_search(reverseGraph,
//      boost::visitor(DfsVisitor(parentMap, false))
//        .root_vertex(endVertex));
//  // Check for a path between endPoint and each register.
//  std::vector<Path> paths;
//  BGL_FORALL_VERTICES(v, graph, Graph) {
//    if (isStartPoint(graph[v])) {
//      auto path = determinePath(parentMap,
//                                Path(),
//                                endVertex,
//                                static_cast<VertexDesc>(graph[v].id));
//      if (!path.empty()) {
//        paths.push_back(path);
//      }
//    }
//  }
//  return paths;
//}
//
/// Report a single path between a set of named points.
VertexIDVec
Graph::getAnyPointToPoint(const VertexIDVec &waypoints) const {
  std::vector<VertexID> path;
  // Construct the path between each adjacent waypoints.
  for (std::size_t i = 0; i < waypoints.size()-1; ++i) {
    auto startVertex = waypoints[i];
    auto endVertex = waypoints[i+1];
    INFO(std::cout << "Performing DFS from "
                   << graph[startVertex].getName() << "\n");
    ParentMap parentMap;
    boost::depth_first_search(graph,
        boost::visitor(DfsVisitor(parentMap, false))
          .root_vertex(startVertex));
    INFO(std::cout << "Determining a path to "
                   << graph[endVertex].getName() << "\n");
    auto subPath = determinePath(parentMap,
                                 VertexIDVec(),
                                 startVertex,
                                 endVertex);
    if (subPath.empty()) {
      // No path exists.
      return VertexIDVec();
    }
    std::reverse(std::begin(subPath), std::end(subPath));
    path.insert(std::end(path), std::begin(subPath), std::end(subPath)-1);
  }
  path.push_back(waypoints.back());
  return path;
}

///// Report all paths between start and end points.
//std::vector<Path> Netlist::
//getAllPointToPoint() const {
//  assert(waypoints.size() > 2 && "invlalid waypoints");
//  INFO(std::cout << "Performing DFS\n");
//  ParentMap parentMap;
//  boost::depth_first_search(graph,
//      boost::visitor(DfsVisitor(parentMap, true))
//        .root_vertex(waypoints[0]));
//  INFO(std::cout << "Determining all paths\n");
//  std::vector<Path> paths;
//  determineAllPaths(parentMap,
//                    paths,
//                    Path(),
//                    waypoints[0],
//                    waypoints[1]);
//  for (auto &path : paths) {
//    std::reverse(std::begin(path), std::end(path));
//  }
//  return paths;
//}
//
///// Return the number of registers a start point fans out to.
//unsigned Netlist::
//getfanOutDegree(VertexDesc startVertex) {
//  const auto paths = getAllFanOut(startVertex);
//  unsigned count = 0;
//  for (auto &path : paths) {
////    auto endVertex = path.back();
////    count += graph[endVertex].width;
//    count += 1;
//  }
//  return count;
//}
//
///// Return he number of registers that fan into an end point.
//unsigned Netlist::
//getFanInDegree(VertexDesc endVertex) {
//  const auto paths = getAllFanIn(endVertex);
//  unsigned count = 0;
//  for (auto &path : paths) {
////    auto startVertex = path.front();
////    count += graph[startVertex].width;
//    count += 1;
//  }
//  return count;
//}
