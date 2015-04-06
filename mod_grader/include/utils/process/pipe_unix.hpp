#ifndef PIPE_UNIX_HPP
#define PIPE_UNIX_HPP

// BOOST headers
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

namespace grader
{
  using pipe_istream = boost::iostreams::stream<boost::iostreams::file_descriptor_source>;
  using pipe_ostream = boost::iostreams::stream<boost::iostreams::file_descriptor_sink>;
  using fd_source = boost::iostreams::file_descriptor_source;
  using fd_sink = boost::iostreams::file_descriptor_sink;
  
  class pipe 
  {
    //////////////////////////////////////////////////////////////////////////////
    // Types and constants
    //////////////////////////////////////////////////////////////////////////////
  public:
    using handle = int;
    
    static constexpr handle invalid_handle = -1;
    static constexpr int read_end = 0;
    static constexpr int write_end = 1;
  
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    handle m_read;
    handle m_write;
    
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Constructors and destructor
    //////////////////////////////////////////////////////////////////////////////
    pipe();
    ~pipe();
    
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    handle get_read_handle() const;
    
    handle get_write_handle() const;
    
    void close_read();
    
    void close_write();   
    
    void close_both();
    
    void redirect_read(handle h) const;
    
    void redirect_write(handle h) const;
  };
}

#endif // PIPE_UNIX_HPP