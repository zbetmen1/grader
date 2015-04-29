// Project headers
#include "network/tcp_server.hpp"
#include "configuration.hpp"
#include "logger.hpp"

// STL headers
#include <functional>

using namespace std;

namespace grader 
{
  tcp_server::tcp_server(aio::io_service& io_service)
  : m_acceptor(io_service, tcp::endpoint(tcp::v4(), stoi(configuration::instance().get($(port)))))
  {
    start_accept();
  }
  
  void tcp_server::start_accept()
  {
    tcp_connection::pointer connection = tcp_connection::create(m_acceptor.get_io_service());
    m_acceptor.async_accept(connection->socket(), 
                            bind(&tcp_server::handle_accept, this, connection, placeholders::_1));
  }
  
  void tcp_server::handle_accept(tcp_connection::pointer newConnection, const sys::error_code& error)
  {
    if (!error)
    {
      newConnection->start();
      start_accept();
    }
    else 
    {
      glog_st.log(severity::error, "Error occurred on server when accepting incoming connection. Message: '", 
                  error.message(), "'.");
      start_accept(); // ???
    }
  }
}