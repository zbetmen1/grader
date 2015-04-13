#ifndef GRADER_INTERFACE_HPP
#define GRADER_INTERFACE_HPP

extern "C" char* submit_new_task(char* sourceName, char* sourceCont, char* test);
extern "C" char* get_task_status(char* taskId);
extern "C" void destroy_task(char* taskId);

void handle_timeout(int);

#endif // GRADER_INTERFACE_HPP