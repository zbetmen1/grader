#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

// STL headers
#include <unordered_map>
#include <string>
#include <cstddef>

// BOOST headers
#include <boost/optional.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace grader
{
  class configuration
  {
  public:
    using map_type = std::unordered_map<std::string, std::string>;
    
    static const std::string SHMEM_NAME;
    static const std::string BASE_DIR;
    static constexpr std::size_t SHMEM_SIZE = 65535U;
  private:
    map_type m_conf;
    static bool s_loaded;
    
    // Forbid construction
    configuration();
  public:
    // Forbid copying and moving
    configuration(const configuration&) = delete;
    configuration& operator=(const configuration&) = delete;
    configuration(configuration&&) = delete;
    configuration& operator=(configuration&&) = delete;
    
    // Singleton's entry point
    static const configuration& instance();
    
    // API
    map_type::const_iterator get(const std::string& key) const;
    
  private:
    void load_config();
  };
  
  boost::interprocess::managed_shared_memory& shm();
  
  extern "C" const void* get_configuration();
}

#endif // CONFIGURATION_HPP