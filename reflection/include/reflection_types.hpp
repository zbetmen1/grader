#ifndef REFLECTION_TYPES
#define REFLECTION_TYPES

#include <string>
#include <unordered_map>
#include <vector>

namespace reflection 
{
  class object;
  typedef void (*object_dtor)(void*);
  typedef object* (*object_ctor)(void);
  typedef void* (*object_method)(void*, ...);
  typedef double (*object_method_real)(void*, ...);
  typedef std::size_t (*object_method_unsigned)(void*, ...);
  
  using shared_lib_impl = void*;
  using path = std::string;
  struct function_pair_names
  {
    std::string cpp_function;
    std::string c_function;
    
    function_pair_names(const std::string& cpp, const std::string& c)
    : cpp_function{cpp}, c_function{c}
    {}
    
    function_pair_names() {}
  };

  using hash_constructors = std::unordered_map<std::string, std::string>;
  using hash_methods = std::unordered_map<std::string, std::vector<function_pair_names>>;
  
  class class_already_exists: std::runtime_error
  {
    std::string m_msg;
  public:
    explicit class_already_exists(const std::string& arg)
    : std::runtime_error{arg}, m_msg{arg}
    {}
    
    virtual const char* what() const noexcept { return m_msg.c_str(); }
  };
}

#endif // REFLECTION_TYPES