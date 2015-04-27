// Project headers
#include "plugin_manager.hpp"
#include "configuration.hpp"
#include "logger.hpp"

// STL headers
#include <sstream>

// BOOST headers
#include <boost/filesystem.hpp>

using namespace std;
namespace fs = boost::filesystem;

namespace grader 
{
  plugin_info::plugin_info()
  {}
  
  plugin_info::plugin_info(const string& lpath)
  : lib(lpath)
  {
    plugin_metadata* metadata = static_cast<plugin_metadata*>(lib.get_c_symbol("metadata"));
    if (!metadata || !metadata->class_name || !metadata->lang_name)
      THROW_SMART(plugin_exception, "No plugin metadata!");
    
    class_name = metadata->class_name;
    lang_name = metadata->lang_name;
  }
  
  plugin_manager::plugin_manager()
  {
    reload();
  }
  
  plugin_manager::~plugin_manager()
  {}
  
  plugin_manager& plugin_manager::instance()
  {
    static plugin_manager plmgr;
    return plmgr;
  }
  
  const plugin_info& plugin_manager::get(const string& langName) const
  {
    plugin_info fakePlInfo{};
    fakePlInfo.lang_name = langName;
    auto foundPlugin = m_plugins.find(fakePlInfo);
    if (foundPlugin == end(m_plugins))
    {
      string msg = "Plugin not found for language '" + langName + "'.";
      THROW_SMART(plugin_exception, msg);
    }
    
    return *foundPlugin;
  }
  
  bool plugin_manager::has_plugin(const string& langName) const
  {
    plugin_info fakePlInfo{};
    fakePlInfo.lang_name = langName;
    auto foundPlugin = m_plugins.find(fakePlInfo);
    return foundPlugin == end(m_plugins) ? false : true;
  }
  
  void plugin_manager::reload()
  {
    // Clear loaded plugins to enable repetitive calling of this function
    m_plugins.clear();
    
    // Get path to plugin dir from configuration
    configuration& conf = configuration::instance();
    fs::path pluginPath = conf.get(configuration::plugin_dir);
    
    // Check that everything is OK with plugin directory
    if (!fs::exists(pluginPath) || !fs::is_directory(pluginPath))
    {
      glog_st.log(severity::fatal, "Invalid plugin directory in configuration (file does not exists or it isn't a directory)!");
      exit(EXIT_FAILURE);
    }
    
    // Iterate through directory and load plugins
    fs::directory_iterator endIt;
    for (fs::directory_iterator it(pluginPath); it != endIt; ++it)
    {
      if (fs::is_regular_file(it->status()))
      {
        // Skip all files that are not shared libraries
        fs::path libPath = it->path();
        if (libPath.extension().string() != "so")
          continue;
        
        // Add new plugin
        try {
          plugin_info info{libPath.string()};
          m_plugins.insert(move(info));
        } catch(const dynamic::shared_lib_load_failed& e) {
          glog_st.log(severity::error, 
                      "Couldn't load shared library on path: '", 
                      libPath.string(),
                      "'. Message: '",
                      e.what(), "'."
                     );
        } catch (const plugin_exception& e2)
        {
          glog_st.log(severity::error, "Bad plugin on path: '",
                      libPath.string(),
                      "'. Message: '", e2.what(), "'.");
        }
      }
    }
  }
}