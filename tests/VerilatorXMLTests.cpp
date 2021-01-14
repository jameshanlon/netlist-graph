#define BOOST_TEST_MODULE verilator_xml_tests

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "tests/definitions.hpp"
#include "TestContext.hpp"
#include "netlist_paths/Utilities.hpp"


/// Verilator cannot inline packages with functions.
BOOST_FIXTURE_TEST_CASE(orphan_package, TestContext) {
  BOOST_CHECK_NO_THROW(compile("orphan_package.sv"));
  BOOST_TEST(!np->isEmpty());
}

/// Verilator cannot inline classes.
BOOST_FIXTURE_TEST_CASE(module_class_no_inline, TestContext) {
  BOOST_CHECK_NO_THROW(compile("module_class_no_inline.sv"));
  BOOST_TEST(!np->isEmpty());
}

/// Verilator cannot inline interfaces.
BOOST_FIXTURE_TEST_CASE(interface_no_inline, TestContext) {
  BOOST_CHECK_NO_THROW(compile("interface_no_inline.sv"));
  BOOST_TEST(np->isEmpty());
}

/// Verilator cannot flatten public modules.
BOOST_FIXTURE_TEST_CASE(module_public_no_inline, TestContext) {
  BOOST_CHECK_NO_THROW(compile("public_module_no_inline.sv"));
  BOOST_TEST(np->isEmpty());
}

/// Verilator cannot flatten public modules.
BOOST_FIXTURE_TEST_CASE(module_no_inline_pragma, TestContext) {
  BOOST_CHECK_NO_THROW(compile("module_no_inline_pragma.sv"));
  BOOST_TEST(np->isEmpty());
}
