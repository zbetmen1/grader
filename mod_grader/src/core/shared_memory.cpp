#include "shared_memory.hpp"
#include "configuration.hpp"
#include "log.hpp"

using namespace std;

namespace grader 
{ 
  shared_memory::shared_memory()
  {
    configuration& conf = configuration::instance();
    m_memory = shm_type(conf.get(configuration::shmem_name), stoul(conf.get(configuration::shmem_size)));
  }

  shared_memory::~shared_memory()
  {
    configuration& conf = configuration::instance();
    boost::interprocess::shared_memory_object::remove(conf.get(configuration::shmem_name));
  }

  shared_memory& shared_memory::instance()
  {
    static shared_memory memory;
    return memory;
  }
  
  void* shared_memory::allocate(size_t size)
  {
    return m_memory.allocate(size);
  }
  
  handle shared_memory::get_handle_from_address(void* address)
  {
    return m_memory.get_handle_from_address(address);
  }
}