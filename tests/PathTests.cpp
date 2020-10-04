#define BOOST_TEST_MODULE path_tests

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "tests/definitions.hpp"
#include "TestContext.hpp"

void checkVarReport(const netlist_paths::Vertex *vertex,
                    const std::string astTypeStr,
                    const std::string dtypeStr,
                    const std::string name) {
  BOOST_TEST(vertex->getAstTypeStr() == astTypeStr);
  BOOST_TEST(vertex->getDTypeStr() == dtypeStr);
  BOOST_TEST(vertex->getName() == name);
}

void checkLogReport(const netlist_paths::Vertex *vertex,
                    const std::string astTypeStr) {
  BOOST_TEST(vertex->getAstTypeStr() == astTypeStr);
}

BOOST_FIXTURE_TEST_CASE(path_exists, TestContext) {
  BOOST_CHECK_NO_THROW(compile("adder.sv"));
  // Check paths between all start and end points are reported.
  auto startPoints = {"i_a", "i_b"};
  auto endPoints = {"o_sum", "o_co"};
  for (auto s : startPoints) {
    for (auto e : endPoints) {
      BOOST_TEST(np->pathExists(netlist_paths::Waypoints(s, e)));
      BOOST_CHECK_THROW(np->pathExists(netlist_paths::Waypoints(e, s)),
                        netlist_paths::Exception);
    }
  }
}

BOOST_FIXTURE_TEST_CASE(path_query_basic_assign_chain, TestContext) {
  BOOST_CHECK_NO_THROW(compile("basic_assign_chain.sv"));
  auto vertices = np->getAnyPath(netlist_paths::Waypoints("in", "out"));
  BOOST_TEST(vertices.size() == 7);
  checkVarReport(vertices[0], "VAR", "logic", "in");
  checkLogReport(vertices[1], "ASSIGN");
  checkVarReport(vertices[2], "VAR", "logic", "basic_assign_chain.a");
  checkLogReport(vertices[3], "ASSIGN");
  checkVarReport(vertices[4], "VAR", "logic", "basic_assign_chain.b");
  checkLogReport(vertices[5], "ASSIGN");
  checkVarReport(vertices[6], "VAR", "logic", "out");
}

BOOST_FIXTURE_TEST_CASE(path_query_basic_comb_chain, TestContext) {
  BOOST_CHECK_NO_THROW(compile("basic_comb_chain.sv"));
  auto vertices = np->getAnyPath(netlist_paths::Waypoints("in", "out"));
  BOOST_TEST(vertices.size() == 7);
  checkVarReport(vertices[0], "VAR", "logic", "in");
  checkLogReport(vertices[1], "ASSIGN");
  checkVarReport(vertices[2], "VAR", "logic", "basic_comb_chain.a");
  checkLogReport(vertices[3], "ASSIGN");
  checkVarReport(vertices[4], "VAR", "logic", "basic_comb_chain.b");
  checkLogReport(vertices[5], "ASSIGN");
  checkVarReport(vertices[6], "VAR", "logic", "out");
}

BOOST_FIXTURE_TEST_CASE(path_query_basic_ff_chain, TestContext) {
  BOOST_CHECK_NO_THROW(compile("basic_ff_chain.sv"));
  // in -> a
  auto vertices = np->getAnyPath(netlist_paths::Waypoints("in", "basic_ff_chain.a"));
  BOOST_TEST(vertices.size() == 3);
  checkVarReport(vertices[0], "VAR", "logic", "in");
  checkLogReport(vertices[1], "ASSIGN_DLY");
  checkVarReport(vertices[2], "DST_REG", "logic", "basic_ff_chain.a");
  // a -> b
  vertices = np->getAnyPath(netlist_paths::Waypoints("basic_ff_chain.a", "basic_ff_chain.b"));
  BOOST_TEST(vertices.size() == 3);
  checkVarReport(vertices[0], "SRC_REG", "logic", "basic_ff_chain.a");
  checkLogReport(vertices[1], "ASSIGN_DLY");
  checkVarReport(vertices[2], "DST_REG", "logic", "basic_ff_chain.b");
  // b -> out
  vertices = np->getAnyPath(netlist_paths::Waypoints("basic_ff_chain.b", "out"));
  BOOST_TEST(vertices.size() == 3);
  checkVarReport(vertices[0], "SRC_REG", "logic", "basic_ff_chain.b");
  checkLogReport(vertices[1], "ASSIGN");
  checkVarReport(vertices[2], "VAR", "logic", "out");
}

BOOST_FIXTURE_TEST_CASE(path_query_pipeline_module, TestContext) {
  BOOST_CHECK_NO_THROW(compile("pipeline_module.sv"));
  // NOTE: can differentiate between the generate instances of the pipeline.
  auto vertices = np->getAnyPath(netlist_paths::Waypoints("i_data", "data_q"));
  BOOST_TEST(vertices.size() == 7);
  checkVarReport(vertices[0], "VAR", "[31:0] logic", "i_data");
  checkLogReport(vertices[1], "ASSIGN");
  checkVarReport(vertices[2], "VAR", "[31:0] logic [8:0]", "pipeline_module.routing");
  checkLogReport(vertices[3], "ASSIGN");
  checkVarReport(vertices[4], "VAR", "[31:0] logic", "pipeline_module.__Vcellinp__g_pipestage[0].u_pipestage__i_data");
  checkLogReport(vertices[5], "ASSIGN_DLY");
  checkVarReport(vertices[6], "DST_REG", "[31:0] logic", "pipeline_module.g_pipestage[0].u_pipestage.data_q");
}

BOOST_FIXTURE_TEST_CASE(path_query_pipeline_loops, TestContext) {
  BOOST_CHECK_NO_THROW(compile("pipeline_loops.sv"));
  // NOTE: cannot currently differentiate between elements of the data_q array.
  auto vertices = np->getAnyPath(netlist_paths::Waypoints("i_data", "data_q"));
  BOOST_TEST(vertices.size() == 3);
  checkVarReport(vertices[0], "VAR", "[31:0] logic", "i_data");
  checkLogReport(vertices[1], "ASSIGN_DLY");
  checkVarReport(vertices[2], "DST_REG", "[31:0] logic [7:0]", "pipeline_loops.data_q");
}

BOOST_FIXTURE_TEST_CASE(path_query_pipeline_no_loops, TestContext) {
  BOOST_CHECK_NO_THROW(compile("pipeline_no_loops.sv"));
  // NOTE: cannot currently differentiate between elements of the data_q array.
  auto vertices = np->getAnyPath(netlist_paths::Waypoints("data_q", "data_q"));
  BOOST_TEST(vertices.size() == 3);
  checkVarReport(vertices[0], "SRC_REG", "[31:0] logic [2:0]", "pipeline_no_loops.data_q");
  checkLogReport(vertices[1], "ASSIGN_DLY");
  checkVarReport(vertices[2], "DST_REG", "[31:0] logic [2:0]", "pipeline_no_loops.data_q");
}

BOOST_FIXTURE_TEST_CASE(path_all_paths, TestContext) {
  BOOST_CHECK_NO_THROW(compile("multiple_paths.sv"));
  auto paths = np->getAllPaths(netlist_paths::Waypoints("in", "out"));
  BOOST_TEST(paths.size() == 3);
  // TODO: check the paths
}

BOOST_FIXTURE_TEST_CASE(path_fan_out, TestContext) {
  BOOST_CHECK_NO_THROW(compile("fan_out_in.sv"));
  auto paths = np->getAllFanOut("in");
  BOOST_TEST(paths.size() == 3);
  // TODO: check the paths
}

BOOST_FIXTURE_TEST_CASE(path_fan_in, TestContext) {
  BOOST_CHECK_NO_THROW(compile("fan_out_in.sv"));
  auto paths = np->getAllFanIn("out");
  BOOST_TEST(paths.size() == 3);
  // TODO: check the paths
}
