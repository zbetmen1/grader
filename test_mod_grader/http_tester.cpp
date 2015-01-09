// Project headers
#include "http_tester.hpp"

// STL headers
#include <iostream>
#include <iterator>
#include <utility>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <fstream>

// BOOST headers
#include <boost/array.hpp>

using namespace std;

namespace grader_test 
{
  http_tester::http_tester(const string& server, const string& base_dir, const string& srcMimeType, const string& url)
  : m_server(server), m_baseDir(base_dir), m_srcMimeType(srcMimeType), m_url(url)
  {
  }

  string http_tester::submit(const string& sourceName, const string& testName)
  {
    using boost::asio::ip::tcp;
    boost::asio::io_service ioService;
    
    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(ioService);
    tcp::resolver::query query(m_server, "http");
    tcp::resolver::iterator endpointIterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(ioService);
    boost::asio::connect(socket, endpointIterator);
    
    // Create request stream and fill data to body stream
    boost::asio::streambuf request;
    ostream requestStream(&request);
    ostringstream bodyStream;
    streampos beginBodyStream = bodyStream.tellp();
    string boundary = fill_request_data(sourceName, testName, bodyStream);
    auto contentLen = bodyStream.tellp() - beginBodyStream;
    
    // Fill request headers and copy body from bodyStream
    requestStream << "POST " << m_url << " HTTP/1.1\r\n";
    requestStream << "Host: " << m_server << "\r\n";
    requestStream << "Accept: */*\r\n";
    requestStream << "Content-Type: " << "multipart/form-data; "
                  << "boundary=" << boundary << "\r\n";
    requestStream << "Content-Length: " << contentLen << "\r\n";
    requestStream << "Connection: close\r\n\r\n";
    requestStream << bodyStream.str();
    
    // Send request and return response body
    boost::asio::write(socket, request);
    return move(body_as_string(socket));
  }

  string http_tester::fetch_status(const string& taskId) const
  {
    using boost::asio::ip::tcp;
    boost::asio::io_service ioService;
    
    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(ioService);
    tcp::resolver::query query(m_server, "http");
    tcp::resolver::iterator endpointIterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(ioService);
    boost::asio::connect(socket, endpointIterator);
    
    // Create request stream
    boost::asio::streambuf request;
    ostream requestStream(&request);
    requestStream << "GET /" << taskId << '.' << "grade " << "HTTP/1.1\r\n";
    requestStream << "Host: " << m_server << "\r\n";
    requestStream << "Accept: */*\r\n";
    requestStream << "Connection: close\r\n\r\n";
    
    // Send request and return response body
    boost::asio::write(socket, request);
    return move(body_as_string(socket));
  }
  
  string http_tester::delete_task(const string& taskId) const
  {
    using boost::asio::ip::tcp;
    boost::asio::io_service ioService;
    
    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(ioService);
    tcp::resolver::query query(m_server, "http");
    tcp::resolver::iterator endpointIterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(ioService);
    boost::asio::connect(socket, endpointIterator);
    
    // Create request stream
    boost::asio::streambuf request;
    ostream requestStream(&request);
    requestStream << "DELETE /" << taskId << '.' << "grade " << "HTTP/1.1\r\n";
    requestStream << "Host: " << m_server << "\r\n";
    requestStream << "Accept: */*\r\n";
    requestStream << "Connection: close\r\n\r\n";
    
    // Send request and return response body
    boost::asio::write(socket, request);
    return move(body_as_string(socket));
  }
  
  string http_tester::fill_request_data(const string& sourceName, const string& testName, ostream& bodyStream) const
  {
    // Create multipart header for source file
    string boundary = create_boundary();
    bodyStream << "--" << boundary << "\r\n";
    bodyStream << "Content-Disposition: form-data; name =\"fileToUpload\"; filename=\"" << sourceName << "\"\r\n";
    bodyStream << "Content-Type: " << m_srcMimeType << "\r\n\r\n";
    
    // Read source file into stream
    ifstream srcFileStream(m_baseDir + '/' + sourceName);
    bodyStream << srcFileStream.rdbuf() << "\r\n";
    
    // Create multipart header for test file
    bodyStream << "--" << boundary << "\r\n";
    bodyStream << "Content-Disposition: form-data; name =\"xmlToUpload\"; filename=\"" << testName << "\"\r\n";
    bodyStream << "Content-Type: " << "text/xml\r\n\r\n";
    
    // Read test file into stream
    ifstream testFileStream(m_baseDir + '/' + testName);
    bodyStream << testFileStream.rdbuf() << "\r\n";
    bodyStream << "--" << boundary << "--";
    return move(boundary);
  }
  
  string http_tester::create_boundary() const
  {
    static constexpr unsigned BOUNDARY_LEN = 64U;
    srand(time(nullptr));
    stringstream boundaryStream;
    for (unsigned i = 0; i < BOUNDARY_LEN; ++i)
    {
      boundaryStream << (rand() % 10);
    }
    return move(boundaryStream.str());
  }

  string http_tester::body_as_string(boost::asio::ip::tcp::socket& socket) const
  {
    // Get response
    boost::asio::streambuf responseBuff;
    boost::asio::read_until(socket, responseBuff, string("\r\n\r\n"));
    istream responseStream(&responseBuff);
    stringstream retStream;
    
    // Skip headers
    string header;
    while (getline(responseStream, header) && header != "\r");
    
    // Read any data beyond headers that is in buffer (it will be part of body)
    if (responseBuff.size() > 0)
      retStream << &responseBuff;
    
    // Read whole body
    boost::system::error_code code;
    while (boost::asio::read(socket, responseBuff, boost::asio::transfer_at_least(1), code))
      std::cout << &responseBuff;
    
    // In case of error throw exception
    if (code != boost::asio::error::eof)
      throw boost::system::system_error(code);
    return move(retStream.str());
  }
}