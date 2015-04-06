#ifndef PLATFORM_SPECIFIC
#define PLATFORM_SPECIFIC

#if defined(__linux__) || defined(__APPLE__)

  #include <cstdlib>
  #include <cstring>
  #include <errno.h>
  #include <dlfcn.h>

#endif

namespace dynamic
{
  namespace platform 
  {
#if defined(__linux__) || defined(__APPLE__)
    
    // Wrap shared library return values
    using shared_lib_impl = void*;
    using shared_lib_sym = void*;
    using shared_lib_c_fun_ptr = void*;
    
    // Wrap open, close and find symbol in shared library
    inline shared_lib_impl open_shared_lib(const char* path, int flag) noexcept { return dlopen(path, flag); }
    inline int close_shared_lib(shared_lib_impl lib) noexcept { return dlclose(lib); }
    inline shared_lib_sym fetch_sym_from_shared_lib(shared_lib_impl lib, const char* sym) noexcept { return dlsym(lib, sym); }
    
    // Wrap error functions
    inline char* shared_lib_error() noexcept { return dlerror(); }
    inline char* error_from_os_code() noexcept { return strerror(errno); }
    
    // Wrap getting full path from path
    inline char* real_path(const char* path) noexcept { return ::realpath(path, nullptr); }
#endif
  }
}
#endif // PLATFORM_SPECIFIC