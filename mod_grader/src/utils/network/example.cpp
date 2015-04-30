#include <ctime>
 #include <iostream>
 #include <string>
 #include <boost/bind.hpp>
 #include <boost/shared_ptr.hpp>
 #include <boost/enable_shared_from_this.hpp>
 #include <boost/asio.hpp>

 using boost::asio::ip::tcp;

 class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
 {
   public:

   typedef boost::shared_ptr<tcp_connection> pointer;

   static pointer create(boost::asio::io_service& io_service)
   {
      return pointer(new tcp_connection(io_service));
   }

   tcp::socket& socket()
   {
      return socket_;
   }

   void start()
   { 
      // Start reading messages from the server
      start_read();

      if (messageFromClient_ == "MESSAGEA:")
      {
         messageToClient_ = "A|A|A|A|A|A|A|A|A|A";
      }

      else if (messageFromClient_ == "MESSAGEB:")
      {
         messageToClient_ = "B|B|B|B|B|B|B|B|B|B";
      }

      boost::asio::async_write(socket_, boost::asio::buffer(messageToClient_),
          boost::bind(&tcp_connection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
   }

   private:

   tcp_connection(boost::asio::io_service& io_service) : socket_(io_service)
   {
   }

   // Reading messages from the server
   void start_read()
   {
      // Start an asynchronous operation to read a newline-delimited message.
      // When read, handle_read should kick in
      boost::asio::async_read(socket_, input_buffer_,
          boost::asio::transfer_at_least(1),
          boost::bind(&tcp_connection::handle_read, this,
          boost::asio::placeholders::error));
   }

   // When stream is received, handle the message from the client
   void handle_read(const boost::system::error_code& ec)
   {
      //if (stopped_)
      //  return;

      // Making the message nil every time the function starts. Do I need it???????
      messageFromClient_ = "";

      if (!ec)
      {
          // Extract the newline-delimited message from the buffer.
          std::string line;
          std::istream is(&input_buffer_);
          std::getline(is, line);

          // Empty messages are heartbeats and so ignored.
          if (!line.empty())
          {
              messageFromClient_ = line;
              std::cout << "Received: " << line << "\n";
          }

          start_read();
       }
       else
       {
        std::cout << "Error on receive: " << ec.message() << "\n";
       }
   }

   void handle_write(const boost::system::error_code& /*error*/,
     size_t /*bytes_transferred*/)
   {
   }

   tcp::socket socket_;
   std::string messageToClient_;
   boost::asio::streambuf input_buffer_;
   std::string messageFromClient_;
 };



 class tcp_server
 {
 public:

 tcp_server(boost::asio::io_service& io_service)
   : acceptor_(io_service, tcp::endpoint(tcp::v4(), 7767))
 {
    start_accept();
 }

 private:

 void start_accept()
 {
    tcp_connection::pointer new_connection =
        tcp_connection::create(acceptor_.get_io_service());

    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&tcp_server::handle_accept, this, new_connection,
        boost::asio::placeholders::error));
 }

 void handle_user_read(const boost::system::error_code& err,
    std::size_t bytes_transferred)
 {
 }

 void handle_accept(tcp_connection::pointer new_connection,
    const boost::system::error_code& error)
 {
    if (!error)
    {
       new_connection->start();

       start_accept();
    }
 }

 tcp::acceptor acceptor_;
 };


 int main()
 {
    try
    {
       boost::asio::io_service io_service;
       tcp_server server(io_service);
       io_service.run();
    }
    catch (std::exception& e)
    {
       std::cerr << e.what() << std::endl;
    }

    return 0;
  }