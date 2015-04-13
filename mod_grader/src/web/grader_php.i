%module grader_php

%{
  extern char* submit_new_task(char* sourceName, char* sourceCont, char* test);
  extern char* get_task_status(char* taskId);
  extern void destroy_task(char* taskId);
%}

extern char* submit_new_task(char* sourceName, char* sourceCont, char* test);
extern char* get_task_status(char* taskId);
extern void destroy_task(char* taskId);