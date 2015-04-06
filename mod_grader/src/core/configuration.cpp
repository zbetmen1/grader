// Project headers
#include "configuration.hpp"
#include "log.hpp"

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
const configuration::grader_info configuration::INVALID_GR_INFO{"", ""};

// Runtime configurable parameters
const string configuration::SHMEM_NAME = "SHMEM_NAME";
const string configuration::BASE_DIR = "BASE_DIR";
const string configuration::SHMEM_SIZE = "SHMEM_SIZE";
const string configuration::LIB_DIR = "LIB_DIR";
const string configuration::SHELL = "SHELL";
const string configuration::SHELL_CMD_FLAG = "SHELL_CMD_FLAG";
const string configuration::LOG_DIR= "LOG_DIR";
const string configuration::LOG_FILE= "LOG_FILE";
const string configuration::LOG_LEVEL = "LOG_LEVEL";

configuration::configuration()
{
  load_config();
}

configuration::~configuration()
{
}

configuration::map_type::const_iterator configuration::get(const std::string& key) const noexcept
{
  return m_conf.find(key);
}

configuration::grader_info configuration::get_grader(const string& languageName) const noexcept
{
  auto it = m_languages.find(language(languageName, "", ""));
  if (m_languages.cend() == it)
    return INVALID_GR_INFO;
  else 
    return move(make_pair(it->grader_name, it->lib_name));
}

void configuration::load_config()
{
  // Read XML configuration into property tree
  ptree pt;
  ifstream in(PATH_TO_CONFIG_FILE);
  if (!in.is_open())
  {
    exit(1);
  }
  xml_parser::read_xml(in, pt, xml_parser::no_comments);
  if (in.bad())
  {
    exit(1);
  }
  else if (in.fail())
  {
    exit(1);
  }
  
  // Iterate over property tree and fill map for configuration
  auto root = pt.get_child("config");
  auto treeItBegin = root.begin();
  auto treeItEnd = root.end();
  while (treeItBegin != treeItEnd)
  {
    if (treeItBegin->first != "LANGUAGE")
    {
      if (m_conf.cend() == m_conf.find(treeItBegin->first))
      {
        m_conf[treeItBegin->first] = treeItBegin->second.get_value<string>();
      }
    }
    else 
    {
      string name;
      string graderName;
      string libName;
      auto nameChild = treeItBegin->second.get_child_optional("NAME");
      auto graderChild = treeItBegin->second.get_child_optional("GRADER");
      auto libChild = treeItBegin->second.get_child_optional("LIB");
      if (nameChild) name = nameChild->get_value<string>();
      if (graderChild) graderName = graderChild->get_value<string>();
      if (libChild) libName = libChild->get_value<string>();
      m_languages.emplace(name, graderName, libName);
    }
    ++treeItBegin;
  }
  
  // Mark that configuration is loaded
  s_loaded = true;
}

const configuration& configuration::instance() noexcept
{
  static configuration singleton;
  return singleton;
}

const string& configuration::get_grader_name(const configuration::grader_info& grInfo) noexcept
{
  return grInfo.first;
}

const string& configuration::get_lib_name(const configuration::grader_info& grInfo) noexcept
{
  return grInfo.second;
}

}