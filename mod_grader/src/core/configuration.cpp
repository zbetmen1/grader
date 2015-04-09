// Project headers
#include "configuration.hpp"

// STL headers
#include <iterator>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

namespace grader 
{
  CONFIG_DEF(configuration_path);
  CONFIG_DEF(shmem_name);
  CONFIG_DEF(shmem_size);
  CONFIG_DEF(jail_dir);
  CONFIG_DEF(work_dir);
  CONFIG_DEF(plugin_dir);
  CONFIG_DEF(jail_user_base_name);
  CONFIG_DEF(source_base_dir);
  
  configuration::configuration()
  {
    reload(configuration_path);
  }
  
  const string& configuration::get(const string& key) const
  {
    auto keyIt = m_conf.find(key);
    if (keyIt == end(m_conf))
      throw configuration_exception("Invalid key!");
    
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
      throw configuration_exception("Couldn't open configuration file for reading!");
    
    for (string line; getline(in, line); )
    {
      if (line[0] == '#')
        continue;
      
      auto equalityPos = line.find('=');
      if (equalityPos == string::npos)
        throw configuration_exception("Invalid configuration, '=' was not found in one of lines!");
      
      string key = line.substr(0, equalityPos);
      string val = line.substr(equalityPos + 1);
      
      trim(key);
      trim(val);
      
      m_conf.insert(move(key), move(val));
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
    ltrim(rtrim(str));
  }

}