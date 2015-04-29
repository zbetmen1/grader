#ifndef TCP_CONNECTION
#define TCP_CONNECTION

// STL headers
#include <memory>
#include <string>

// BOOST headers
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
namespace aio = boost::asio;
namespace sys = boost::system;

namespace grader 
{
  class tcp_connection: public std::enable_shared_from_this<tcp_connection> 
  {
  public:
    //////////////////////////////////////////////////////////////////////////////
    // Types
    //////////////////////////////////////////////////////////////////////////////
    using pointer = std::shared_ptr<tcp_connection>;
    
    enum command {
      save_tests = 0,
      submit_task = 1,
      query_task_status = 2
    };
    
  private:
    //////////////////////////////////////////////////////////////////////////////
    // Members
    //////////////////////////////////////////////////////////////////////////////
    tcp::socket m_socket;
    aio::streambuf m_buffer;
    std::string m_response;
    
    //////////////////////////////////////////////////////////////////////////////
    // Creators and destructor
    //////////////////////////////////////////////////////////////////////////////
    tcp_connection(aio::io_service& io_service);
  public:
    static pointer create(aio::io_service& io_service);
   
    //////////////////////////////////////////////////////////////////////////////
    // Operations
    //////////////////////////////////////////////////////////////////////////////
    tcp::socket& socket();
    
    void start();
  private:
    
    void execute_save_tests(std::istream& input);
    
    void execute_submit_task(std::istream& input);
    
    void execute_query_task_status(std::istream& input);
    
    void handle_write(const sys::error_code&, size_t);
  };
}

#endif // TCP_CONNECTION