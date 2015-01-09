#ifndef HTTP_TESTER_HPP
#define HTTP_TESTER_HPP

// STL headers
#include <string>

// BOOST headers
#include <boost/asio.hpp>

namespace grader_test
{
  class http_tester 
  {
    std::string m_server;
    std::string m_baseDir;
    std::string m_srcMimeType;
    std::string m_url;
  public:
    http_tester(const std::string& server, const std::string& base_dir, const std::string& srcMimeType, 
                const std::string& url = "/upload.grade");
    
    std::string submit(const std::string& sourceName, const std::string& testName);
    std::string fetch_status(const std::string& taskId) const;
    std::string delete_task(const std::string& taskId) const;
    
    // Server get/set
    const std::string& server() const { return m_server; }
    std::string& server() { return m_server; }
    
    // Base dir get/set
    const std::string& base_dir() const { return m_baseDir; }
    std::string& base_dir() { return m_baseDir; }
    
    // Source file mime type
    const std::string& src_mime_type() const { return m_srcMimeType; }
    std::string& src_mime_type() { return m_srcMimeType; }
    
    // URL get/set
    const std::string& url() const { return m_url; }
    std::string& url() { return m_url; }
  private:
    std::string fill_request_data(const std::string& sourceName, const std::string& testName, std::ostream& bodyStream) const;
    std::string create_boundary() const;
    std::string body_as_string(boost::asio::ip::tcp::socket& socket) const;
  };
}

#endif // HTTP_TESTER_HPP