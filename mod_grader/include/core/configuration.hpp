#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

// Project headers
#include "grader_log.hpp"

// STL headers
#include <unordered_map>
#include <string>
#include <cstddef>
#include <unordered_set>

// BOOST headers
#include <boost/interprocess/managed_shared_memory.hpp>

struct language;


namespace grader
{
  struct language 
  {
    std::string name, grader_name, lib_name;
    language(const std::string& name_, const std::string& grader_name_, const std::string& lib_name_)
    : name(name_), grader_name(grader_name_), lib_name(lib_name_)
    {}
  };
}

namespace std 
{
  template<> struct hash<grader::language>
  {
    size_t operator()(const grader::language& l) const
    {
      return hash<string>()(l.name);
    }
  };
  
  template<> struct equal_to<grader::language>
  {
    bool operator()(const grader::language& l, const grader::language& r) const
    {
      return l.name == r.name;
    }
  };
}

namespace grader 
{
  class configuration
  {
  public:
    // Types
    using map_type = std::unordered_map<std::string, std::string>;
    using grader_info = std::pair<std::string, std::string>;
    
    // Compile time parameters
    static const std::string PATH_TO_CONFIG_FILE;
    static const grader_info INVALID_GR_INFO;
    
    // Runtime parameters
    static const std::string SHELL;
    static const std::string SHELL_CMD_FLAG;
    static const std::string SHMEM_NAME;
    static const std::string SHMEM_SIZE;
    static const std::string BASE_DIR;
    static const std::string LIB_DIR;
    static const std::string LOG_DIR;
    static const std::string LOG_FILE;
    static const std::string LOG_LEVEL;
  private:
    map_type m_conf;
    std::unordered_set<language> m_languages;
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
    static const configuration& instance() noexcept;
    
    // API
    map_type::const_iterator get(const std::string& key) const noexcept;
    map_type::const_iterator invalid() const noexcept { return m_conf.cend(); }
    grader_info get_grader(const std::string& languageName) const noexcept;
    static const std::string& get_grader_name(const grader_info& grInfo) noexcept;
    static const std::string& get_lib_name(const grader_info& grInfo) noexcept;
  private:
    void load_config();
  };
  
  boost::interprocess::managed_shared_memory& shm();
  
  template <typename T>
  void shm_destroy(const char* name)
  {
    shm().destroy<T>(name);
  }
  
  template <typename T>
  T* shm_find(const char* name)
  {
    auto found = shm().find<T>(name);
    return 0 != found.second ? found.first : nullptr;
  }
}

#endif // CONFIGURATION_HPP