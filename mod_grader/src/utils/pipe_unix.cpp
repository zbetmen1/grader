// Project headers
#include "pipe.hpp"

// Unix headers
#include <unistd.h>
#include <cstring>
#include <errno.h>

namespace grader
{
  pipe::pipe()
  : m_read(invalid_handle), m_write(invalid_handle)
  {
    handle fd[2];
    if (::pipe(fd) != -1)
    {
      m_read = fd[read_end];
      m_write = fd[write_end];
    }
  }
  
  pipe::~pipe()
  {}
  
  pipe::handle pipe::get_read_handle() const
  {
    return m_read;
  }

  pipe::handle pipe::get_write_handle() const
  {
    return m_write;
  }
  
  void pipe::close_read()
  {
    if (m_read != invalid_handle)
    {
      ::close(m_read);
      m_read = invalid_handle;
    }
  }
  
  void pipe::close_write()
  {
    if (m_write != invalid_handle)
    {
      ::close(m_write);
      m_write = invalid_handle;
    }
  }

  void pipe::close_both()
  {
    close_read();
    close_write();
  }
  
  void pipe::redirect_read(pipe::handle h) const
  {
    if (::dup2(m_read, h) == -1)
      throw pipe_exception(::strerror(errno));
  }
  
  void pipe::redirect_write(pipe::handle h) const
  {
    if (::dup2(m_write, h) == -1)
      throw pipe_exception(::strerror(errno));
  }
}