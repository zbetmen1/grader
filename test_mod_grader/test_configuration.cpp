#include <boost/test/unit_test.hpp>

#include "../mod_grader/include/core/configuration.hpp"

using namespace grader;

BOOST_AUTO_TEST_SUITE(test_configuration)

BOOST_AUTO_TEST_CASE(Construction)
{
  configuration& conf = configuration::instance();
}

BOOST_AUTO_TEST_SUITE_END()