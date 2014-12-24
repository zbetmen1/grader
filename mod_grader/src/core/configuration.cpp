// Project headers
#include "configuration.hpp"

// STL headers
#include <fstream>

// BOOST headers
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

using namespace std;
using namespace boost::property_tree;
using namespace boost::log;

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
const string configuration::LOG_DIR= "LOG_DIR";
const string configuration::LOG_FILE= "LOG_FILE";
const string configuration::LOG_LEVEL = "LOG_LEVEL";

configuration::configuration()
{
  load_config();
  init_logging();
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

void configuration::init_logging() const
{
  add_file_log(keywords::file_name = get(LOG_DIR)->second + '/' + get(LOG_FILE)->second, 
               keywords::rotation_size = 10 * 1024 * 1024,
               keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
               keywords::format = "[%TimeStamp%]: %Message%"
              );
  string logLevel = get(LOG_LEVEL)->second;
  if ("TRACE" == logLevel)
  {
    core::get()->set_filter(trivial::severity >= trivial::trace);
  }
  else if ("DEBUG" == logLevel)
  {
    core::get()->set_filter(trivial::severity >= trivial::debug);
  }
  else if ("INFO" == logLevel)
  {
    core::get()->set_filter(trivial::severity >= trivial::info);
  }
  else if ("WARNING" == logLevel)
  {
    core::get()->set_filter(trivial::severity >= trivial::warning);
  }
  else if ("ERROR" == logLevel)
  {
    core::get()->set_filter(trivial::severity >= trivial::error);
  }
  else if ("FATAL" == logLevel)
  {
    core::get()->set_filter(trivial::severity >= trivial::fatal);
  }
  else 
  {
    core::get()->set_filter(trivial::severity >= trivial::info);
  }
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