#ifndef AUTOCALL_HPP
#define AUTOCALL_HPP

// STL headers
#include <functional>

namespace grader 
{
  class autocall 
  {
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    std::function<void()> m_call;
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    template <typename Callable, typename ...Args>
    autocall(Callable f, Args... args)
    : m_call(std::bind(f, args...))
    {}
    
    autocall()
    : m_call()
    {}
    
    // Copy-construction and copy-assign is forbidden
    autocall(const autocall&) = delete;
    autocall& operator=(const autocall&) = delete;
    
    // Move-construction and move-assign is available
    autocall(autocall&& oth)
    : m_call(oth.m_call)
    {
      oth.m_call = std::function<void()>{};
    }
    
    autocall& operator=(autocall&& oth)
    {
      if (&oth != this)
      {
        m_call = oth.m_call;
        oth.m_call = std::function<void()>{};
      }
      return *this;
    }
    
    bool is_callable() const { return m_call ? true : false; }
    
    void release() noexcept { m_call = std::function<void()>{}; }
    
    void fire() 
    { 
      auto f = std::function<void()>{};
      std::swap(f, m_call);
      f(); 
    }
    
    ~autocall() 
    { 
      if (is_callable())
        fire();
    }
  };
}

#endif // AUTOCALL_HPP