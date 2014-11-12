// Project headers
#include "configuration.hpp"

using namespace std;

namespace grader
{
  bool configuration::s_loaded = false;
  
  const string configuration::SHMEM_NAME = "SHMEM_NAME";
  const string configuration::BASE_DIR = "BASE_DIR";
  
  configuration::configuration()
  {}

  configuration::map_type::const_iterator configuration::get(const std::string& key) const
  {
    return m_conf.find(key);
  }

  void configuration::load_config()
  {
    // TODO: Read all configs from file in fixed location.
    
    s_loaded = true;
  }
  
  const configuration& configuration::instance()
  {
    static configuration singleton;
    if (!s_loaded) singleton.load_config();
    
    return singleton;
  }
  
  boost::interprocess::managed_shared_memory& shm()
  {
    static boost::interprocess::managed_shared_memory sshm(boost::interprocess::open_or_create, 
                                                           configuration::instance().get(configuration::SHMEM_NAME)->second.c_str(), 
                                                           configuration::SHMEM_SIZE);
    return sshm;
  }
  
  const void* get_configuration()
  {
    return &configuration::instance();
  }
}