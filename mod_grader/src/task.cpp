// Project headers
#include "task.hpp"
#include "configuration.hpp"

// STL headers
#include <algorithm>
#include <utility>

// BOOST headers
#include <boost/interprocess/sync/scoped_lock.hpp>

using namespace std;

namespace grader
{
  task::mutex_type s_lock;
  
  task::task(const std::string& pathToSrc, const std::vector< task::test >& tests, const std::string& id)
  : m_pathToSrc(shm().get_segment_manager()), m_tests(shm().get_segment_manager()), m_id(shm().get_segment_manager()),
  m_state(state::WAITING), m_status(shm().get_segment_manager())
  {
    // Copy path to src file
    m_pathToSrc.reserve(pathToSrc.size());
    copy(pathToSrc.cbegin(), pathToSrc.cend(), m_pathToSrc.begin());
    
    // Copy tests
    m_tests.reserve(tests.size());
    copy(tests.cbegin(), tests.cend(), m_tests.begin());
    
    // Copy uuid
    m_id.reserve(id.size());
    copy(id.cbegin(), id.cend(), m_id.begin());
  }
  
  const task::shm_uuid& task::id() const
  {
    return m_id;
  }

  void task::run_all() const
  {

  }
  
  std::string task::status() const
  {
    boost::interprocess::scoped_lock<mutex_type> lock(s_lock);
    std::string retval;
    switch(m_state)
    {
      case state::WAITING:
        retval = "{ \"STATE\" : \"WAITING\" }";
        return move(retval);
      case state::COMPILING:
        retval = "{ \"STATE\" : \"COMPILING\" }";
        return move(retval);
      case state::RUNNING:
        retval = "{ \"STATE\" : \"RUNNING\" }";
        return move(retval);
      case state::FINISHED:
        retval.reserve(m_status.size()); // yet written to m_status, we need to lock this section
        copy(m_status.cbegin(), m_status.cend(), retval.begin());
        return move(retval);
    }
    return move(retval);
  }
  
  void task::set_state(task::state newState)
  {
    boost::interprocess::scoped_lock<mutex_type> lock(s_lock);
    m_state = newState;
  }

}