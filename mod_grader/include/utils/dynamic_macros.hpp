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
        
#endif // DYNAMIC_MACROS_HPP