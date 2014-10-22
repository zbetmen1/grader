#ifndef DYNAMIC_MACROS_HPP
#define DYNAMIC_MACROS_HPP

/**
 * This section of file contains utility macros that are used across whole file
 * to ease development of more complex macros.
 **/

// Define common names
#define METHODS_SUPPORT_OBJ msobj
#define GEN_WRAPPER_NAME(ClassName, MethodName) __dynamic_##ClassName##MethodName

// Stringification
#define EXPANDE_QUOTE(X) QUOTE(X)
#define QUOTE(X) #X

// Count number of elements in variadic macro 
#define NARGS_SEQ(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
#define NARGS(...) NARGS_SEQ(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

// Force macro expansion before concatenation
#define PRIMITIVE_CAT(x, y) x ## y
#define CAT(x, y) PRIMITIVE_CAT(x, y)

/**
 * This section of file contains macros for simple object-derived class registration.
 **/

// Generates proper constructor and destructor C wrappers (note handling of std::bad_alloc)
#define C_CTOR_DTOR_BODIES(ClassName) \
  extern "C" \
  void __dynamic_destroy_##ClassName(void* obj) \
  { \
    ClassName* realObj = static_cast<ClassName*>(obj); \
    delete realObj; \
  } \
  extern "C" \
  void* __dynamic_create_##ClassName() \
  { \
    ClassName* newObj = nullptr; \
    try \
    { \
      newObj = new ClassName{}; \
    } \
    catch (const std::bad_alloc&) \
    { \
      return nullptr; \
    } \
    return static_cast<void*>(newObj); \
  }

// Single-threaded object-derived class registration
#define REGISTER_DYNAMIC_ST(ClassName) \
  C_CTOR_DTOR_BODIES(ClassName) \
  std::unique_ptr<dynamic::register_creators> __dynamic_object_##ClassName{new dynamic::register_creators{QUOTE(ClassName), \
                                                                                                         &__dynamic_create_##ClassName, \
                                                                                                         &__dynamic_destroy_##ClassName}};
// Multithreaded object-derived class registration
#define REGISTER_DYNAMIC_MT(ClassName) \
  C_CTOR_DTOR_BODIES(ClassName) \
  std::unique_ptr<dynamic::register_creators> __dynamic_object_##ClassName{new dynamic::register_creators{QUOTE(ClassName), \
                                                                                                         &__dynamic_create_##ClassName, \
                                                                                                         &__dynamic_destroy_##ClassName, \
                                                                                                         true }};

/**
 * This section of file contains macros needed for registration of class that will be able to fire
 * method calls even if these methods aren't virtual, but are registered. Also here are macros
 * that are used to wrap these calls in C API functions.
 **/

// Generate proper function call from types of arguments
#define GEN_ARGS(...) CAT(GEN_ARGS_, NARGS(__VA_ARGS__))(__VA_ARGS__)
#define GEN_ARGS_1(x1) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0])
#define GEN_ARGS_2(x1, x2) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0]), any_cast<x2>(METHODS_SUPPORT_OBJ->get_arguments()[1])
#define GEN_ARGS_3(x1, x2, x3) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0]), any_cast<x2>(METHODS_SUPPORT_OBJ->get_arguments()[1]), any_cast<x3>(METHODS_SUPPORT_OBJ->get_arguments()[2])
#define GEN_ARGS_4(x1, x2, x3, x4) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0]), any_cast<x2>(METHODS_SUPPORT_OBJ->get_arguments()[1]), any_cast<x3>(METHODS_SUPPORT_OBJ->get_arguments()[2]), any_cast<x4>(METHODS_SUPPORT_OBJ->get_arguments()[3])
#define GEN_ARGS_5(x1, x2, x3, x4, x5) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0]), any_cast<x2>(METHODS_SUPPORT_OBJ->get_arguments()[1]), any_cast<x3>(METHODS_SUPPORT_OBJ->get_arguments()[2]), any_cast<x4>(METHODS_SUPPORT_OBJ->get_arguments()[3]), any_cast<x5>(METHODS_SUPPORT_OBJ->get_arguments()[4])
#define GEN_ARGS_6(x1, x2, x3, x4, x5, x6) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0]), any_cast<x2>(METHODS_SUPPORT_OBJ->get_arguments()[1]), any_cast<x3>(METHODS_SUPPORT_OBJ->get_arguments()[2]), any_cast<x4>(METHODS_SUPPORT_OBJ->get_arguments()[3]), any_cast<x5>(METHODS_SUPPORT_OBJ->get_arguments()[4]), any_cast<x6>(METHODS_SUPPORT_OBJ->get_arguments()[5])
#define GEN_ARGS_7(x1, x2, x3, x4, x5, x6, x7) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0]), any_cast<x2>(METHODS_SUPPORT_OBJ->get_arguments()[1]), any_cast<x3>(METHODS_SUPPORT_OBJ->get_arguments()[2]), any_cast<x4>(METHODS_SUPPORT_OBJ->get_arguments()[3]), any_cast<x5>(METHODS_SUPPORT_OBJ->get_arguments()[4]), any_cast<x6>(METHODS_SUPPORT_OBJ->get_arguments()[5]), any_cast<x7>(METHODS_SUPPORT_OBJ->get_arguments()[6])
#define GEN_ARGS_8(x1, x2, x3, x4, x5, x6, x7, x8) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0]), any_cast<x2>(METHODS_SUPPORT_OBJ->get_arguments()[1]), any_cast<x3>(METHODS_SUPPORT_OBJ->get_arguments()[2]), any_cast<x4>(METHODS_SUPPORT_OBJ->get_arguments()[3]), any_cast<x5>(METHODS_SUPPORT_OBJ->get_arguments()[4]), any_cast<x6>(METHODS_SUPPORT_OBJ->get_arguments()[5]), any_cast<x7>(METHODS_SUPPORT_OBJ->get_arguments()[6]), any_cast<x8>(METHODS_SUPPORT_OBJ->get_arguments()[7])
#define GEN_ARGS_9(x1, x2, x3, x4, x5, x6, x7, x8, x9) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0]), any_cast<x2>(METHODS_SUPPORT_OBJ->get_arguments()[1]), any_cast<x3>(METHODS_SUPPORT_OBJ->get_arguments()[2]), any_cast<x4>(METHODS_SUPPORT_OBJ->get_arguments()[3]), any_cast<x5>(METHODS_SUPPORT_OBJ->get_arguments()[4]), any_cast<x6>(METHODS_SUPPORT_OBJ->get_arguments()[5]), any_cast<x7>(METHODS_SUPPORT_OBJ->get_arguments()[6]), any_cast<x8>(METHODS_SUPPORT_OBJ->get_arguments()[7]), any_cast<x9>(METHODS_SUPPORT_OBJ->get_arguments()[8])
#define GEN_ARGS_10(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10) any_cast<x1>(METHODS_SUPPORT_OBJ->get_arguments()[0]), any_cast<x2>(METHODS_SUPPORT_OBJ->get_arguments()[1]), any_cast<x3>(METHODS_SUPPORT_OBJ->get_arguments()[2]), any_cast<x4>(METHODS_SUPPORT_OBJ->get_arguments()[3]), any_cast<x5>(METHODS_SUPPORT_OBJ->get_arguments()[4]), any_cast<x6>(METHODS_SUPPORT_OBJ->get_arguments()[5]), any_cast<x7>(METHODS_SUPPORT_OBJ->get_arguments()[6]), any_cast<x8>(METHODS_SUPPORT_OBJ->get_arguments()[7]), any_cast<x9>(METHODS_SUPPORT_OBJ->get_arguments()[8]), any_cast<x10>(METHODS_SUPPORT_OBJ->get_arguments()[9])

// Macro that wraps method call with C function (no exception can go through)
#define WRAP_DYNAMIC_METHOD(ClassName, MethodName, ...) \
  extern "C" \
  void GEN_WRAPPER_NAME(ClassName, MethodName)(void* obj) \
  { \
    using namespace dynamic; \
    ClassName* METHODS_SUPPORT_OBJ = static_cast<ClassName*>(obj); \
    try \
    { \
      METHODS_SUPPORT_OBJ->result() = METHODS_SUPPORT_OBJ->MethodName(GEN_ARGS(__VA_ARGS__)); \
    } catch (const std::bad_cast&) \
    { \
      METHODS_SUPPORT_OBJ->result() = std::string("Bad parameter types given to function '" #MethodName "'!"); \
    } catch(const std::exception& e) \
    { \
      METHODS_SUPPORT_OBJ->result() = std::string(e.what()); \
    } catch(...) \
    { \
      METHODS_SUPPORT_OBJ->result() = std::string("Unknown error!"); \
    } \
  }

// Apply bind will apply macro to all of it's arguments with first arguments binded as a first macro argument
#define APPLY_BIND(macro, binded, ...) CAT(APPLY_BIND_, NARGS(__VA_ARGS__))(macro, binded, __VA_ARGS__)
#define APPLY_BIND_1(m, binded, x1) m(binded, x1)
#define APPLY_BIND_2(m, binded, x1, x2) m(binded, x1), m(binded, x2)
#define APPLY_BIND_3(m, binded, x1, x2, x3) m(binded, x1), m(binded, x2), m(binded, x3)
#define APPLY_BIND_4(m, binded, x1, x2, x3, x4) m(binded, x1), m(binded, x2), m(binded, x3), m(binded, x4)
#define APPLY_BIND_5(m, binded, x1, x2, x3, x4, x5) m(binded, x1), m(binded, x2), m(binded, x3), m(binded, x4), m(binded, x5)
#define APPLY_BIND_6(m, binded, x1, x2, x3, x4, x5, x6) m(binded, x1), m(binded, x2), m(binded, x3), m(binded, x4), m(binded, x5), m(binded, x6)
#define APPLY_BIND_7(m, binded, x1, x2, x3, x4, x5, x6, x7) m(binded, x1), m(binded, x2), m(binded, x3), m(binded, x4), m(binded, x5), m(binded, x6), m(binded, x7)
#define APPLY_BIND_8(m, binded, x1, x2, x3, x4, x5, x6, x7, x8) m(binded, x1), m(binded, x2), m(binded, x3), m(binded, x4), m(binded, x5), m(binded, x6), m(binded, x7), m(binded, x8)

// Generates pair of names where first is C++ method name and second is C wrapper name
#define GET_FUNC_PAIR(ClassName, MethodName) {QUOTE(MethodName), EXPANDE_QUOTE(GEN_WRAPPER_NAME(ClassName, MethodName))} 

// Single-threaded methods_support-derived class registration
#define REGISTER_DYNAMIC_METHODS_ST(ClassName, ...) \
  REGISTER_DYNAMIC_ST(ClassName) \
  std::unique_ptr<dynamic::register_methods> __dynamic_methods_##ClassName{ \
      new dynamic::register_methods{QUOTE(ClassName), \
                                    { APPLY_BIND(GET_FUNC_PAIR, ClassName, __VA_ARGS__)}}};

// Multithreaded methods_support-derived class registration
#define REGISTER_DYNAMIC_METHODS_MT(ClassName, ...) \
  REGISTER_DYNAMIC_MT(ClassName) \
  std::unique_ptr<dynamic::register_methods> __dynamic_methods_##ClassName{ \
      new dynamic::register_methods{QUOTE(ClassName), \
                                    { APPLY_BIND(GET_FUNC_PAIR, ClassName, __VA_ARGS__)}, true}};
                                    
#endif // DYNAMIC_MACROS_HPP