// Project headers
#include "network/tcp_connection.hpp"
#include "logger.hpp"
#include "configuration.hpp"
#include "autocall.hpp"

// STL headers
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

// BOOST headers
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace fs = boost::filesystem;
namespace uuids = boost::uuids;
using namespace std;

namespace grader 
{
  tcp_connection::tcp_connection(aio::io_service& io_service)
  : m_socket(io_service)
  {}
  
  tcp_connection::pointer tcp_connection::create(aio::io_service& io_service)
  {
    return pointer(new tcp_connection(io_service));
  }
  
  tcp::socket& tcp_connection::socket()
  {
    return m_socket;
  }
  
  void tcp_connection::start()
  {
    // Read all data
    sys::error_code err;
    aio::read(m_socket, m_buffer, aio::transfer_all(), err);
    
    // Check if an error occurred
    if (err)
    {
      glog_st.log(severity::error, "Network error! Message: '", err.message(), "'.");
      return;
    }
    
    // Parse data
    istream input(&m_buffer);
    int cmdInt;
    command cmd;
    if (!(input >> cmdInt))
    {
      glog_st.log(severity::error, "Reading command from stream failed!");
      return;
    }
    cmd = static_cast<command>(cmdInt);
    
    // Dispatch on command
    if (cmd == command::save_tests)
      execute_save_tests(input);
    else if (cmd == command::submit_task)
      execute_submit_task(input);
    else if (cmd == command::query_task_status)
      execute_query_task_status(input);
    else 
      glog_st.log(severity::error, "Unknown command given: '", cmd, "'!");
  }
  
  void tcp_connection::execute_save_tests(istream& input)
  {
    // Initialize safety
    autocall safety = [&]()
    {
      m_response = "0";
      aio::async_write(m_socket, aio::buffer(m_response), bind(&tcp_connection::handle_write, this, placeholders::_1, placeholders::_2));
    };
    
    // Read path
    fs::path testPath;
    if (!(input >> testPath))
    {
      glog_st.log(severity::error, "Couldn't read path to test file (stream failed)!");
      return;
    }
    
    // Check if path is relative
    if (!testPath.is_relative())
    {
      glog_st.log(severity::error, "Given path isn't relative! Path: '", testPath.string(), "'.");
      return;
    }  
    
    // Create directories if they not exist
    if (testPath.has_parent_path())
    {
      // Construct absolute path
      configuration& conf = configuration::instance();
      fs::path dirPath = conf.get($(tests_base_dir));
      dirPath /= testPath.parent_path();
      
      // Create directories
      sys::error_code err;
      fs::create_directories(dirPath, err);
      if (err)
      {
        glog_st.log(severity::error, "Failed to create directories to store test file in. Message: '",
                    err.message(), "'.");
        return;
      }
    }
    
    // Copy all data to file
    ofstream out(testPath.string());
    if (!out)
    {
      glog_st.log(severity::error, "Couldn't open file stream to write test file to!");
      return;
    }
    copy(istreambuf_iterator<char>(input), istreambuf_iterator<char>(), ostreambuf_iterator<char>(out));
    
    // Release safety and inform client of success
    safety.release();
    m_response = "1";
    aio::async_write(m_socket, aio::buffer(m_response), bind(&tcp_connection::handle_write, this, placeholders::_1, placeholders::_2));
  }

  void tcp_connection::execute_submit_task(istream& input)
  {
    // Initialize safety
    autocall safety = [&]()
    {
      m_response = "0";
      aio::async_write(m_socket, aio::buffer(m_response), bind(&tcp_connection::handle_write, this, placeholders::_1, placeholders::_2));
    };
    
    // Read test path from stream
    fs::path testPath;
    string sourceName;
    if (!(input >> testPath))
    {
      glog_st.log(severity::error, "Couldn't read path to test file from network stream.");
      return;
    }
    
    // Construct absolute path to test file
    fs::path absoluteTestPath;
    if (testPath.is_relative())
    {
      configuration& conf = configuration::instance();
      absoluteTestPath = conf.get($(tests_base_dir));
      absoluteTestPath /= testPath;
    }
    else 
    {
      absoluteTestPath = testPath;
    }
    
    // Check if test file exists
    sys::error_code err;
    if (!fs::exists(absoluteTestPath, err))
    {
      glog_st.log(severity::error, "Test file doesn't exists or error happened! Message: '", 
                  (err ? err.message() : ""), "'. Path: '", absoluteTestPath.string(), "'.");
      return;
    }
    
    // Read source file name from stream
    if (!(input >> sourceName))
    {
      glog_st.log(severity::error, "Couldn't read path to source file from network stream.");
      return;
    }
    
    // TODO: Create task, release safety and return task id to client
    safety.release();
  }
  
  void tcp_connection::execute_query_task_status(istream& input)
  {
    // Initialize safety
    autocall safety = [&]()
    {
      m_response = "0";
      aio::async_write(m_socket, aio::buffer(m_response), bind(&tcp_connection::handle_write, this, placeholders::_1, placeholders::_2));
    };
    
    // Get task id
    uuids::uuid taskId;
    if (!(input >> taskId))
    {
      glog_st.log(severity::error, "Failed to read task id from network stream.");
      return;
    }
    
    // TODO: Query and return task status using task id, release safety
    safety.release();
  }
  
  void tcp_connection::handle_write(const sys::error_code& ec, size_t bytesTransfered)
  {
    if (ec)
    {
      glog_st.log(severity::error, "Writing result back to client failed! Bytes transfered: ",
        bytesTransfered, ". Message: '", ec.message(), "'.");
    }
  }

  
}