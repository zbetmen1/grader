#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

// STL headers
#include <unordered_map>
#include <string>
#include <cstddef>
#include <unordered_set>
#include <tuple>

// BOOST headers
#include <boost/interprocess/managed_shared_memory.hpp>

struct language;


namespace grader
{
  struct language 
  {
    std::string name, grader_name, lib_name, interpreter_path;
    language(const std::string& name_, const std::string& grader_name_, const std::string& lib_name_, 
             const std::string& interpreter_path_ = "")
    : name(name_), grader_name(grader_name_), lib_name(lib_name_), interpreter_path(interpreter_path_)
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
    using grader_info = std::tuple<std::string, std::string, std::string>;
    
    // Compile time parameters
    static const std::string PATH_TO_CONFIG_FILE;
    
    // Runtime parameters
    static const std::string SHMEM_NAME;
    static const std::string BASE_DIR;
    static const std::string SHMEM_SIZE;
    static const std::string LIB_DIR;
    static const std::string SHELL;
    static const std::string SHELL_CMD_FLAG;
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
    static const configuration& instance();
    
    // API
    map_type::const_iterator get(const std::string& key) const;
    grader_info get_grader(const std::string& languageName) const;
    static const std::string& get_grader_name(const grader_info& grInfo);
    static const std::string& get_lib_name(const grader_info& grInfo);
    static const std::string& get_interpreter_path(const grader_info& grInfo);
  private:
    void load_config();
  };
  
  boost::interprocess::managed_shared_memory& shm();
  
  extern "C" const void* get_configuration();
}

#endif // CONFIGURATION_HPP