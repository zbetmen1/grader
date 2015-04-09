#ifndef SHARED_LIB_H
#define SHARED_LIB_H

// STL headers
#include <string>
#include <stdexcept>
#include <type_traits>

// Project headers
#include "object.hpp"
#include "platform_specific.hpp"

namespace dynamic
{ 
  
  class shared_lib_load_failed: public std::runtime_error
  {
  public:
    explicit shared_lib_load_failed(const char* arg)
    : std::runtime_error{arg}
    {}
  };
  
  // Forward declaration of object to minimize compilation dependencies
  class object;
  
  enum class shared_lib_mode: int
  {
    LAZY = RTLD_LAZY,
    NOW = RTLD_NOW
  };
  
  class shared_lib
  {
    platform::shared_lib_impl m_impl;
    
  public:
    shared_lib();
    shared_lib(const std::string& p, shared_lib_mode flag = shared_lib_mode::LAZY);
    ~shared_lib();
    
    platform::shared_lib_c_fun_ptr 
    get_c_symbol(const std::string& functionName) const;
    
    // Shared library shouldn't be copied
    shared_lib(const shared_lib&) = delete;
    shared_lib& operator=(const shared_lib&) = delete;
    
    // Shared library should be movable
    shared_lib(shared_lib&& moved);
    shared_lib& operator=(shared_lib&& moved);
    
    template <typename Derived>
    std::unique_ptr<Derived, object_dtor> make_object(const std::string& className)
    {
      // Get constructor pointer class name
      auto ctor = object::constructor(className);
      
      // Create new object
      Derived* dobj = static_cast<Derived*>((*ctor)());
      return std::unique_ptr<Derived, object_dtor>{dobj, object::destructor(className)};
    }
  };
}

#endif // SHARED_LIB_H
