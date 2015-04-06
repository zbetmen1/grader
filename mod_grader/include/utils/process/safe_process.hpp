#ifndef SAFE_PROCESS_HPP
#define SAFE_PROCESS_HPP

#if defined(__linux__) || defined(__APPLE__)
#include "safe_process_unix.hpp"
#endif // Unix

#endif // SAFE_PROCESS_HPP