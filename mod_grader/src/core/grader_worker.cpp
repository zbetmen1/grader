#include "configuration.hpp"
#include "task.hpp"
#include "interprocess_queue.hpp"
#include "grader_log.hpp"

#include <string>

#include <boost/interprocess/offset_ptr.hpp>

using namespace grader;
using namespace std;

template <typename T>
using shm_ptr = boost::interprocess::offset_ptr<T>;
using task_queue = interprocess_queue<shm_ptr<task>>;

int main()
{
  task_queue* q = grader::shm().find_or_construct<task_queue>("TaskQueue")(shm());
  while (true) 
  {
    shm_ptr<task> currentTask = nullptr;
    q->pop(currentTask);
    
    if (currentTask)
    {
      LOG(string("Working on task: ") + currentTask->id(), grader::INFO);
      currentTask->run_all();
    }
  }
}