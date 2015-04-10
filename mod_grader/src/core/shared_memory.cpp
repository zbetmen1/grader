#include "shared_memory.hpp"
#include "configuration.hpp"

using namespace std;
namespace ip = boost::interprocess;

namespace grader 
{ 
  shared_memory::shared_memory()
  {
    configuration& conf = configuration::instance();
    m_memory = shm_type{ip::open_or_create,
                        conf.get(configuration::shmem_name).c_str(), 
                        stoul(conf.get(configuration::shmem_size))};
  }

  shared_memory::~shared_memory()
  {
    configuration& conf = configuration::instance();
    boost::interprocess::shared_memory_object::remove(conf.get(configuration::shmem_name).c_str());
  }

  shared_memory& shared_memory::instance()
  {
    static shared_memory memory;
    return memory;
  }
}