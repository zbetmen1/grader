// BOOST headers
#include <boost/test/unit_test.hpp>

// Project headers
#include "../mod_grader/include/core/configuration.hpp"

// STL headers
#include <string>

using namespace std;
using namespace grader;

BOOST_AUTO_TEST_SUITE(test_configuration)

BOOST_AUTO_TEST_CASE(Construction)
{
  configuration& conf = configuration::instance();
}

BOOST_AUTO_TEST_CASE(GetFromKey)
{
  configuration& conf = configuration::instance();
  string shmname = conf.get($(shmem_name));
  BOOST_CHECK(shmname == "GraderShmem01");
  string shmsize = conf.get($(shmem_size));
  BOOST_CHECK(shmsize == "134217728");
  cout << conf.get($(work_dir)) << endl;
}

BOOST_AUTO_TEST_CASE(HasKey)
{
  configuration& conf = configuration::instance();
  BOOST_CHECK(conf.has_key($(shmem_name)));
  BOOST_CHECK(!conf.has_key("zbetmen"));
}

BOOST_AUTO_TEST_SUITE_END()