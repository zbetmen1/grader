// Project headers
#include "configuration.hpp"
#include "logger.hpp"

// STL headers
#include <iterator>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <utility>

using namespace std;

namespace grader 
{
  const string configuration::configuration_path = "/etc/grader/configuration.txt";
  CONFIG_DEF(shmem_name);
  CONFIG_DEF(shmem_size);
  CONFIG_DEF(jail_dir);
  CONFIG_DEF(work_dir);
  CONFIG_DEF(plugin_dir);
  CONFIG_DEF(jail_user_base_name);
  CONFIG_DEF(source_base_dir);
  CONFIG_DEF(log_facility);
  
  configuration::configuration()
  {
    reload(configuration_path);
  }
  
  const string& configuration::get(const string& key) const
  {
    auto keyIt = m_conf.find(key);
    if (keyIt == end(m_conf))
    {
      string msg = "Unknown configuration: '" + key + "'!";
      THROW_SMART(configuration_exception, msg);
    }
    
    return keyIt->second;
  }
  
  bool configuration::has_key(const string& key) const
  {
    auto keyIt = m_conf.find(key);
    return keyIt == end(m_conf) ? false : true;
  }
  
  void configuration::reload(const string& pathToConf)
  {
    ifstream in(pathToConf);
    if (!in)
    {
      glog_st.log(severity::fatal, "Couldn't open configuration file '",
                  configuration_path,
                  "'! Is file there? Are permissions right?");
      exit(EXIT_FAILURE);
    }
    
    unsigned line_num = 1;
    for (string line; getline(in, line); ++line_num)
    {
      if (line[0] == '#')
        continue;
      
      // Check if line is meaningful configuration line
      auto equalityPos = line.find('=');
      if (equalityPos == string::npos)
      {
        // If line isn't comment (checked at beginning of the loop) and isn't empty then we have config error
        if (line.find_first_not_of(" \t\r\n") != string::npos)
          glog_st.log(severity::error, "Configuration in wrong format at line ", line_num, '.');
        continue;
      }
      
      string key = line.substr(0, equalityPos);
      string val = line.substr(equalityPos + 1);
      
      trim(key);
      trim(val);
      
      m_conf.emplace(move(key), move(val));
    }
  }

  configuration& configuration::instance()
  {
    static configuration conf;
    return conf;
  }
  
  void ltrim(string& str)
  {
    auto pos = str.find_first_not_of(" \r\n\t");
    auto newSize = str.size() - pos;
    if (pos != string::npos)
    {
      copy(begin(str) + pos, end(str), begin(str));
      str.erase(newSize);
    }
  }
  
  void rtrim(string& str)
  {
    auto pos = str.find_last_not_of(" \r\n\t");
    if (pos != string::npos && pos < str.size() - 1)
      str.erase(pos+1);
  }

  void trim(string& str)
  {
    rtrim(str);
    ltrim(str);
  }

}