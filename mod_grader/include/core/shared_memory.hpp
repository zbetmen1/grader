#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

// STL headers
#include <string>
#include <cstddef>
#include <cstdint>

// BOOST headers
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/offset_ptr.hpp>

namespace grader 
{
  class shared_memory 
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    using shm_type = boost::interprocess::managed_shared_memory;
    template <typename T>
    using shm_allocator = boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>;
    using shm_string = boost::interprocess::basic_string<char, std::char_traits<char>, shm_allocator<char>>;
    template<typename T>
    using shm_vector = boost::interprocess::vector<T, shm_allocator<T>>;
    using shm_stream = boost::interprocess::bufferstream;
    using shm_mutex = boost::interprocess::interprocess_mutex;
    using shm_condition = boost::interprocess::interprocess_condition;
    template <typename T>
    using shm_ptr = boost::interprocess::offset_ptr<T>;
    using handle = shm_type::handle_t;
  private:
    
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    shm_type m_memory;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    explicit shared_memory();
    ~shared_memory();
  public:
    static shared_memory& instance();
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    void* allocate(std::size_t size);
    
    handle get_handle_from_address(void* address);
    
    template<typename T, typename NameT, typename... Args>
    T* find_or_construct(NameT name, Args... args)
    {
      return m_memory.find_or_construct<T>(name)(args...);
    }
    
    template<typename T, typename NameT, typename... Args>
    T* construct(NameT name, Args... args)
    {
      return m_memory.construct<T>(name)(args...);
    }
    
    template<typename T, typename NameT, typename Size, typename... Args>
    T* construct_array(NameT name, Size n, Args... args)
    {
      return m_memory.construct<T>(name)[n](args...);
    }
    
    template<typename T, typename NameT>
    T* find(NameT name) 
    {
      auto found = m_memory.find<T>(name);
      return 0 != found.second ? found.first : nullptr;
    }
    
    template<typename T, typename NameT>
    void destroy(NameT name)
    {
      m_memory.destroy<T>(name);
    }
    
    template<typename T>
    void destroy_ptr(T* data)
    {
      m_memory.destroy_ptr(data);
    }
    
    auto get_segment_manager()
    {
      return m_memory.get_segment_manager();
    }
  };
}

#endif // SHARED_MEMORY_HPP