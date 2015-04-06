#ifndef CONCURENT_QUEUE
#define CONCURENT_QUEUE

#include "shared_memory.hpp"

// STL headers
#include <cstddef>

// BOOST headers
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace grader 
{
  
  template <typename T>
  class interprocess_queue 
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    
    using shm_type = boost::interprocess::managed_shared_memory;
    
    /**
    * @brief Interprocess shared memory allocator for type T.
    * 
    */  
    using shm_allocator = boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>;
    
    /**
    * @brief Deque that keeps elements in interprocess shared memory.
    * 
    */
    using shm_deque = boost::interprocess::deque<T, shm_allocator>;

    /**
    * @brief Interprocess mutex.
    * 
    */  
    using mutex_type = boost::interprocess::interprocess_mutex;

    /**
    * @brief Interprocess condition.
    * 
    */  
    using condition_type = boost::interprocess::interprocess_condition;
    
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    
    /**
    * @brief Double ended queue where data will be kept internally.
    * 
    */
    shm_deque m_data;
    
    /**
    * @brief Mutex to protect concurrent access from multiple processes.
    * 
    */
    mutable mutex_type m_lock;
    
    /**
    * @brief Condition to synchronize data availability between processes.
    * 
    */
    condition_type m_dataReady;
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    
    explicit interprocess_queue(shared_memory& memory);
    explicit interprocess_queue(const shm_deque& queue);
    explicit interprocess_queue(shm_deque&& queue);
    
    interprocess_queue(const interprocess_queue& oth) = default;
    interprocess_queue& operator=(const interprocess_queue& oth) = default;
    
    interprocess_queue(interprocess_queue&& oth);
    interprocess_queue& operator=(interprocess_queue&& oth);
    
    void push(const T& item);
    void push(T&& item);
    
    void pop(T& out);
    bool try_pop(T& out);
    
    bool empty() const;
    std::size_t size() const;
  };

  template <typename T>
  interprocess_queue<T>::interprocess_queue(grader::shared_memory& memory)
  : m_data(memory.get_segment_manager())
  {}
  
  template <typename T>
  interprocess_queue<T>::interprocess_queue(const interprocess_queue<T>::shm_deque& queue)
  : m_data(queue)
  {}
  
  template <typename T>
  interprocess_queue<T>::interprocess_queue(interprocess_queue<T>::shm_deque&& queue)
  : m_data(boost::move(queue))
  {}
  
  template <typename T>
  interprocess_queue<T>::interprocess_queue(interprocess_queue<T>&& oth)
  : m_data(boost::move(oth.m_data)), m_lock(boost::move(oth.m_lock)), m_dataReady(boost::move(oth.m_dataReady))
  {}
  
  template <typename T>
  interprocess_queue<T>& interprocess_queue<T>::operator=(interprocess_queue<T>&& oth)
  {
    if (&oth != this)
    {
      m_data = boost::move(oth.m_data);
      m_lock = boost::move(oth.m_lock);
      m_dataReady = boost::move(oth.m_dataReady);
    }
    return *this;
  }

  template <typename T>
  void interprocess_queue<T>::push(const T& item)
  {
    boost::interprocess::scoped_lock<interprocess_queue<T>::mutex_type> lock(m_lock);
    m_data.push_back(item);
    m_dataReady.notify_one();
  }
  
  template <typename T>
  void interprocess_queue<T>::push(T&& item)
  {
    boost::interprocess::scoped_lock<interprocess_queue<T>::mutex_type> lock(m_lock);
    m_data.push_back(item);
    m_dataReady.notify_one();
  }
  
  template <typename T>
  void interprocess_queue<T>::pop(T& out)
  {
    boost::interprocess::scoped_lock<interprocess_queue<T>::mutex_type> lock(m_lock);
    m_dataReady.wait(lock, [this](){ return !m_data.empty(); });
    out = m_data.front();
    m_data.pop_front();
  }
  
  template <typename T>
  bool interprocess_queue<T>::try_pop(T& out) 
  {
    boost::interprocess::scoped_lock<interprocess_queue<T>::mutex_type> lock(m_lock);
    if (m_data.empty()) 
      return false;
      
    out = m_data.front();
    m_data.pop_front();
    return true;
  }
  
  template <typename T>
  bool interprocess_queue<T>::empty() const
  {
    boost::interprocess::scoped_lock<interprocess_queue<T>::mutex_type> lock(m_lock);
    return m_data.empty();
  }
  
  template <typename T>
  std::size_t interprocess_queue<T>::size() const
  {
    boost::interprocess::scoped_lock<interprocess_queue<T>::mutex_type> lock(m_lock);
    return m_data.size();
  }
}

#endif // CONCURENT_QUEUE