#include "object.hpp"
#include "grader_log.hpp"

#include <vector>

using namespace std;

namespace dynamic 
{ 
  hash_creators object::m_hashCreatorsName;
  mutex object::m_lockHashes;
  
  bool object::insert_creators_st(const char* className, dynamic::object_ctor ctorName, dynamic::object_dtor dtorName)
  {
    if (m_hashCreatorsName.cend() != m_hashCreatorsName.find(className))
      return false;
    
    m_hashCreatorsName[className] = make_pair(ctorName, dtorName);
    return true;
  }

  bool object::insert_creators_mt(const char* className, dynamic::object_ctor ctorName, dynamic::object_dtor dtorName)
  {
    lock_guard<mutex> lockHash{m_lockHashes};
    return insert_creators_st(className, ctorName, dtorName);
  }
  
  void object::erase_creators_st(const char* className)
  {
    m_hashCreatorsName.erase(className);
  }

  void object::erase_creators_mt(const char* className)
  {
    lock_guard<mutex> lockHash{m_lockHashes};
    erase_creators_st(className);
  }
  
  object_dtor object::destructor(const string& className)
  {
    hash_creators::const_iterator destructorIt;
    if (m_hashCreatorsName.cend() != (destructorIt = m_hashCreatorsName.find(className)))
    {
      return destructorIt->second.second;
    }
    
    glog::error() << "No destructor for class name: " << className << ".\n";
    return nullptr;
  }
  
  object_ctor object::constructor(const std::string& className)
  {
    hash_creators::const_iterator constructorIt;
    if (m_hashCreatorsName.cend() != (constructorIt = m_hashCreatorsName.find(className)))
    {
      return constructorIt->second.first;
    }
    
    glog::error() << "No constructor for class name: " << className << ".\n";
    return nullptr;
  }
  
  object::object()
  { }
  
  object::~object() {}
  
}