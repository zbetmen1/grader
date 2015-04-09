#ifndef TASK_QUEUE_HPP
#define TASK_QUEUE_HPP

// Project headers
#include "interprocess_queue.hpp"
#include "task.hpp"
#include "shared_memory.hpp"

namespace grader 
{
  using task_queue = interprocess_queue<boost::interprocess::offset_ptr<task>>;
  
  task_queue* get_task_queue()
  {
    task_queue* tq = shared_memory::instance().find_or_construct<task_queue>(boost::interprocess::unique_instance, 
                                                                             shared_memory::instance());
    return tq;
  }
}

#endif // TASK_QUEUE_HPP