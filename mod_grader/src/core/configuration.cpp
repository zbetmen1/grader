// Project headers
#include "configuration.hpp"

// STL headers
#include <fstream>

// BOOST headers
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace std;
using namespace boost::property_tree;

namespace grader
{

// Static flags
bool configuration::s_loaded = false;

// Compile time parameters
const string configuration::PATH_TO_CONFIG_FILE="/etc/grader/config.xml";

// Runtime configurable parameters
const string configuration::SHMEM_NAME = "SHMEM_NAME";
const string configuration::BASE_DIR = "BASE_DIR";
const string configuration::SHMEM_SIZE = "SHMEM_SIZE";
const string configuration::LIB_DIR = "LIB_DIR";
const string configuration::SHELL = "SHELL";
const string configuration::SHELL_CMD_FLAG = "SHELL_CMD_FLAG";

configuration::configuration()
{
  load_config();
}

configuration::map_type::const_iterator configuration::get(const std::string& key) const
{
  return m_conf.find(key);
}

configuration::grader_info configuration::get_grader(const string& languageName) const
{
  auto it = m_languages.find(language(languageName, "", ""));
  if (m_languages.cend() == it)
    return move(make_pair<string, string>("",""));
  else 
    return move(make_pair(it->grader_name, it->lib_name));
}

void configuration::load_config()
{
  // Read XML configuration into property tree
  ptree pt;
  ifstream in(PATH_TO_CONFIG_FILE);
  xml_parser::read_xml(in, pt, xml_parser::no_comments);
  
  // Iterate over property tree and fill map for configuration
  auto root = pt.get_child("config");
  auto treeItBegin = root.begin();
  auto treeItEnd = root.end();
  while (treeItBegin != treeItEnd)
  {
    if (treeItBegin->first != "LANGUAGE")
      m_conf[treeItBegin->first] = treeItBegin->second.get_value<string>();
    else 
    {
      string name = treeItBegin->second.get_child("NAME").get_value<string>();
      string graderName = treeItBegin->second.get_child("GRADER").get_value<string>();
      string libName = treeItBegin->second.get_child("LIB").get_value<string>();
      m_languages.emplace(name, graderName, libName);
    }
    ++treeItBegin;
  }
  
  // Mark that configuration is loaded
  s_loaded = true;
}

const configuration& configuration::instance()
{
  static configuration singleton;
  return singleton;
}

boost::interprocess::managed_shared_memory& shm()
{
  static boost::interprocess::managed_shared_memory sshm(boost::interprocess::open_or_create, 
                                                          configuration::instance().get(configuration::SHMEM_NAME)->second.c_str(), 
                                                          stoul(configuration::instance().get(configuration::SHMEM_SIZE)->second));
  return sshm;
}

const void* get_configuration()
{
  return &configuration::instance();
}

const string& configuration::get_grader_name(const configuration::grader_info& grInfo)
{
  return ::get<0>(grInfo);
}

const string& configuration::get_lib_name(const configuration::grader_info& grInfo)
{
  return ::get<1>(grInfo);
}

}