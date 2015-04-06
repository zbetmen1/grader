#include "shared_memory.hpp"
#include "configuration.hpp"
#include "log.hpp"

using namespace std;

namespace grader 
{
  
  const char* shared_memory::SHM_NAME = "GraderShm01";
  
  shared_memory::shared_memory()
  : m_memory(boost::interprocess::open_or_create, SHM_NAME, stoul(configuration::instance().get(configuration::SHMEM_SIZE)->second))
  {}

  shared_memory::~shared_memory()
  {
    boost::interprocess::shared_memory_object::remove(SHM_NAME);
  }

  shared_memory& shared_memory::instance()
  {
    static shared_memory memory;
    return memory;
  }
}