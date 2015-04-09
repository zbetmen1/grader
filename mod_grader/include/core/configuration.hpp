#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

// STL headers
#include <unordered_map>
#include <string>
#include <stdexcept>

#define CONFIG_DECL(name) static const std::string name
#define CONFIG_DEF(name) const std::string configuration::name = #name

namespace grader 
{
  class configuration_exception: public std::runtime_error 
  {
  public:
    explicit configuration_exception(const std::string& arg)
    : std::runtime_error(arg)
    {}
  };
  
  class configuration 
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    template <typename Key, typename Val>
    using map_type = std::unordered_map<Key, Val>;
    
    //////////////////////////////////////////////////////////////////////////////
    // Constants
    //////////////////////////////////////////////////////////////////////////////
    CONFIG_DECL(configuration_path);
    CONFIG_DECL(shmem_name);
    CONFIG_DECL(shmem_size);
    CONFIG_DECL(jail_dir);
    CONFIG_DECL(work_dir);
    CONFIG_DECL(plugin_dir);
    CONFIG_DECL(jail_user_base_name);
    CONFIG_DECL(source_base_dir);
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    map_type<std::string, std::string> m_conf;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    explicit configuration();
    configuration(const configuration&) = delete;
    configuration& operator=(const configuration&) = delete;
    ~configuration() = default;
  public:
    configuration(configuration&& oth) = default;
    configuration& operator=(configuration&& oth) = default;
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    const std::string& get(const std::string& key) const;
    
    bool has_key(const std::string& key) const;
    
  private:
    void reload(const std::string& pathToConf);
    
  public:
    static configuration& instance();
  };
  
  void ltrim(std::string& str);
  
  void rtrim(std::string& str);
  
  void trim(std::string& str);
}

#endif // CONFIGURATION_HPP