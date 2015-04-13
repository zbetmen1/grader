#include "grader_interface.hpp"
#include "task.hpp"
#include "configuration.hpp"

#include <map>
#include <cstddef>
#include <stdexcept>
#include <csignal>
#include <errno.h>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

using namespace std;
using namespace grader;

pid_t g_grandchildPid = -1;
task* g_task = nullptr;

char* submit_new_task(char* sourceName, char* sourceCont, char* test)
{
  pair<size_t, size_t> timeMemReq;
  task* nextTask = task::create_task(sourceName, 
                                     strlen(sourceName), 
                                     sourceCont, 
                                     strlen(sourceCont), 
                                     test, strlen(test), &timeMemReq);
  pid_t childPid = ::fork();
  if (childPid == -1)
    return nullptr;
  else if (childPid)
  {
    const char* tid = nextTask->id();
    char* tidHeap = (char*) calloc(strlen(tid) + 1, sizeof(char));
    strcpy(tidHeap, tid);
    return tidHeap;
  }
  else 
  {
    ::daemon(0, 0);
    g_grandchildPid = ::fork();
    if (g_grandchildPid == -1)
      throw runtime_error("Second fork failed so no grandchild!");
    else if (g_grandchildPid)
    {
      g_task = nextTask;
      if (::signal(SIGALRM, &handle_timeout) == SIG_ERR)
        throw runtime_error(::strerror(errno));
      struct itimerval timer;
      timer.it_interval.tv_sec = 0;
      timer.it_interval.tv_usec = 0;
      timer.it_value.tv_sec = timeMemReq.first / 1000;
      timer.it_value.tv_usec = (timeMemReq.second % 1000) * 1000 ;
      if (::setitimer(ITIMER_REAL, &timer, nullptr) == -1)
        throw runtime_error(::strerror(errno));
      pause();
    }
    else 
    {
      nextTask->run_all();
      exit(EXIT_SUCCESS);
    }
  }
  return nullptr;
}

char* get_task_status(char* taskId)
{
  auto t = shm_find<task>(taskId);
  if (t)
  {
    const char* stat = t->status();
    char* statHeap = (char*) calloc(strlen(stat) + 1, sizeof(char));
    strcpy(statHeap, stat);
    return statHeap;
  }
  return nullptr;
}

void destroy_task(char* taskId)
{
  shm_destroy<task>(taskId);
}

void handle_timeout(int)
{
  int status;
  int result = ::waitpid(g_grandchildPid, &status, WNOHANG);
  if (result == -1) // Error
  {
    throw runtime_error(::strerror(errno));
  }
  else if (!result) // Child still running
  {
    if (::kill(SIGKILL, g_grandchildPid) == -1)
    {
      throw runtime_error(::strerror(errno));
    }
    g_task->set_state(task::state::TIME_LIMIT);
    
    g_task = nullptr;
    g_grandchildPid = -1;
  }
  exit(EXIT_SUCCESS);
}
