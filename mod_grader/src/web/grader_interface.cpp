#include "grader_interface.hpp"
#include "task.hpp"
#include "configuration.hpp"
#include <signal.h>
#include <sys/wait.h>

using namespace std;
using namespace grader;

void handle_child(int)
{
  waitpid(-1, 0, WNOHANG);
}

char* submit_new_task(char* sourceName, char* sourceCont, char* test)
{
  task* nextTask = task::create_task(sourceName, 
                                     strlen(sourceName), 
                                     sourceCont, 
                                     strlen(sourceCont), 
                                     test, strlen(test));
  signal(SIGCHLD, &handle_child);
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
    nextTask->run_all();
    exit(EXIT_SUCCESS);
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
