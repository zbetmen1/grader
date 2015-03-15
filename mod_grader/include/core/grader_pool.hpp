#ifndef GRADER_POOL_HPP
#define GRADER_POOL_HPP

// Project headers
#include "shared_memory.hpp"
#include "interprocess_queue.hpp"
#include "task.hpp"

// STL headers
#include <cstdint>
#include <cstddef>
#include <string>

// BOOST headers
#include <boost/interprocess/offset_ptr.hpp>

// Poco headers
#include <Poco/Process.h>

namespace grader 
{
  class grader_pool 
  { 
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    struct grader_pool_impl 
    {
      //////////////////////////////////////////////////////////////////////////////
      // Members
      //////////////////////////////////////////////////////////////////////////////
      interprocess_queue<shared_memory::shm_ptr<task>> m_data;
      uint32_t m_refcount;
      
      //////////////////////////////////////////////////////////////////////////////
      // Creators and destructor
      //////////////////////////////////////////////////////////////////////////////
      explicit grader_pool_impl();
      ~grader_pool_impl();
      
      grader_pool_impl(const grader_pool_impl&) = delete;
      grader_pool_impl& operator=(const grader_pool_impl&) = delete;
      grader_pool_impl(grader_pool_impl&&) = delete;
      grader_pool_impl& operator=(grader_pool_impl&&) = delete;
      
      //////////////////////////////////////////////////////////////////////////////
      // Operations
      //////////////////////////////////////////////////////////////////////////////
      void submit(task* newTask);
      void get_next(shared_memory::shm_ptr<task>& next);
      
      uint32_t ref_inc();
      uint32_t ref_dec();
      
      std::size_t children_count() const;
      void start_children();
      void stop_children();
    };
    
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    grader_pool_impl* m_impl;
  
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    grader_pool();
    ~grader_pool();
    
    grader_pool(const grader_pool&) = delete;
    grader_pool& operator=(const grader_pool&) = delete;
    grader_pool(grader_pool&&) = delete;
    grader_pool& operator=(grader_pool&&) = delete;
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    static grader_pool& instance();
    
    void submit(task* newTask);
    void get_next(shared_memory::shm_ptr<task>& next);
  };
}

#endif // GRADER_POOL_HPP