// Project headers
#include "grader_pool.hpp"
#include "grader_log.hpp"

// STL headers
#include <vector>

// BOOST headers
#include <boost/interprocess/detail/atomic.hpp>

using namespace std; 

namespace grader 
{
  shared_memory& shm = shared_memory::instance();
  
  grader_pool::grader_pool_impl::grader_pool_impl()
  : m_data(shm), m_refcount(0)
  {
    start_children();
  }
  
  grader_pool::grader_pool_impl::~grader_pool_impl()
  {
    stop_children();
  }
  
  void grader_pool::grader_pool_impl::start_children()
  {
  }

  void grader_pool::grader_pool_impl::stop_children()
  {
  }

  size_t grader_pool::grader_pool_impl::children_count() const
  {
    return 0;
  }
  
  void grader_pool::grader_pool_impl::submit(task* newTask)
  {
    m_data.push(newTask);
  }
  
  void grader_pool::grader_pool_impl::get_next(shared_memory::shm_ptr< task >& next)
  {
    m_data.pop(next);
  }
  
  uint32_t grader_pool::grader_pool_impl::ref_inc()
  {
    return boost::interprocess::ipcdetail::atomic_inc32(&m_refcount);
  }
  
  uint32_t grader_pool::grader_pool_impl::ref_dec()
  {
    return boost::interprocess::ipcdetail::atomic_dec32(&m_refcount);
  }
  
  grader_pool::grader_pool()
  : m_impl(nullptr)
  {
     m_impl = shm.find_or_construct<grader_pool_impl>("JebeniPointer");
     glog::debug() << "(constructor) Grader pool reference count: " << (m_impl->ref_inc() + 1) 
                   << ". Pid: " << getpid() << ".\n";
  }
  
  grader_pool::~grader_pool()
  {
    uint32_t childrenCount = m_impl->ref_dec() - 1;
    glog::debug() << "(destructor) Grader pool reference count: " << childrenCount
                  << ". Pid: " << getpid() << ".\n";
    if (childrenCount == m_impl->children_count())
    {
      shm.destroy_ptr<grader_pool_impl>(m_impl);
      glog::debug() << "Grader pool impl pointer destroyed!\n";
    }
  }
  
  grader_pool& grader_pool::instance()
  {
    static grader_pool gp;
    return gp;
  }
  
  void grader_pool::get_next(shared_memory::shm_ptr< task >& next)
  {
    m_impl->get_next(next);
  }
  
  void grader_pool::submit(task* newTask)
  {
    m_impl->submit(newTask);
  }
}