#ifndef PLUGIN_MANAGER_HPP
#define PLUGIN_MANAGER_HPP

// Project headers
#include "dynamic/shared_lib.hpp"

// STL headers
#include <string>
#include <cstddef>
#include <functional>
#include <unordered_set>
#include <stdexcept>

#define PLUGIN_METADATA(cname, lname) extern "C" { \
  plugin_metadata metadata = {#cname, #lname};

  
extern "C" 
{
  typedef struct 
  {
    const char* class_name;
    const char* lang_name;
  } plugin_metadata;
}

namespace grader 
{
  class plugin_exception: public std::runtime_error 
  {
  public:
    explicit plugin_exception(const std::string& arg)
    : std::runtime_error(arg)
    {}
  };
  
  struct plugin_info 
  {
    dynamic::shared_lib lib;
    std::string class_name;
    std::string lang_name;
    
    explicit plugin_info();
    explicit plugin_info(const std::string& lpath);
  }; 
}

namespace std 
{
  template <> struct hash<plugin_info>
  {
    std::size_t operator()(const grader::plugin_info& plg)
    {
      return std::hash<std::string>{}(plg.lang_name);
    }
  };
  
  template <> struct equal_to<plugin_info>
  {
    bool operator()(const grader::plugin_info& lhs, const grader::plugin_info& rhs)
    {
      return lhs.lang_name == rhs.lang_name;
    }
  };
}

namespace grader 
{
    
  class plugin_manager 
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    template <typename T>
    using set_type = std::unordered_set<T>;
    
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    set_type<plugin_info> m_plugins;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    plugin_manager();
    plugin_manager(const plugin_manager&) = delete;
    plugin_manager& operator=(const plugin_manager&) = delete;
    plugin_manager(plugin_manager&& oth) = default;
    plugin_manager& operator=(plugin_manager&& oth) = default;
    ~plugin_manager();
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
  public:
    static plugin_manager& instance();
    
    const plugin_info& get(const std::string& langName) const;
    
    bool has_plugin(const std::string& langName) const;
    
  private:
    void reload();
  };
}

#endif // PLUGIN_MANAGER_HPP