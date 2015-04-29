#include "shared_memory.hpp"
#include "configuration.hpp"
#include "logger.hpp"

using namespace std;
namespace ip = boost::interprocess;

namespace grader 
{ 
  shared_memory::shared_memory()
  {
    configuration& conf = configuration::instance();
    try {
    m_memory = shm_type{ip::open_or_create,
                        conf.get($(shmem_name)).c_str(), 
                        stoul(conf.get($(shmem_size)))};
    } catch(const std::exception& e) {
      glog_st.log(severity::fatal, "Failed to instantiate shared memory! Reason: '",
                  e.what(), "'.");
      exit(EXIT_FAILURE);
    }
  }

  shared_memory::~shared_memory()
  {
    configuration& conf = configuration::instance();
    ip::shared_memory_object::remove(conf.get($(shmem_name)).c_str());
  }

  shared_memory& shared_memory::instance()
  {
    static shared_memory memory;
    return memory;
  }
}