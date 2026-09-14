#include <fc/network/tcp_socket.hpp>
#include <fc/network/ip.hpp>
#include <fc/network/tcp_socket_io_hooks.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/asio.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/stdio.hpp>
#include <fc/exception/exception.hpp>

#if defined _WIN32 || defined WIN32 || defined OS_WIN64 || defined _WIN64 || defined WIN64 || defined WINNT
# include <mstcpip.h>
#endif

#if defined __OpenBSD__
# include <sys/types.h>
# include <sys/sysctl.h>
# include <netinet/tcp_timer.h>
# include <netinet/tcp_var.h>
#endif

namespace fc {

  class tcp_socket::impl : public tcp_socket_io_hooks {
    public:
      impl() :
        _sock(fc::asio::default_io_service()),
        _io_hooks(this)
      {}
      ~impl()
      {
        if( _sock.is_open() ) 
          try
          {
            _sock.close();
          }
          catch( ... )
          {}
        if( _read_in_progress.valid() )
          try 
          { 
            _read_in_progress.wait();
          }
          catch ( ... )
          {
          }
        if( _write_in_progress.valid() )
          try 
          { 
            _write_in_progress.wait();
          } 
          catch ( ... )
          {
          }
      }
      virtual size_t readsome(boost::asio::ip::tcp::socket& socket, char* buffer, size_t length) override;
      virtual size_t readsome(boost::asio::ip::tcp::socket& socket, const std::shared_ptr<char>& buffer, size_t length, size_t offset) override;
      virtual size_t writesome(boost::asio::ip::tcp::socket& socket, const char* buffer, size_t length) override;
      virtual size_t writesome(boost::asio::ip::tcp::socket& socket, const std::shared_ptr<const char>& buffer, size_t length, size_t offset) override;

      fc::future<size_t> _write_in_progress;
      fc::future<size_t> _read_in_progress;
      boost::asio::ip::tcp::socket _sock;
      tcp_socket_io_hooks* _io_hooks;
  };

  size_t tcp_socket::impl::readsome(boost::asio::ip::tcp::socket& socket, char* buffer, size_t length)
  {
    return (_read_in_progress = fc::asio::read_some(socket, buffer, length)).wait();
  }
  size_t tcp_socket::impl::readsome(boost::asio::ip::tcp::socket& socket, const std::shared_ptr<char>& buffer, size_t length, size_t offset)
  {
    return (_read_in_progress = fc::asio::read_some(socket, buffer, length, offset)).wait();
  }
  size_t tcp_socket::impl::writesome(boost::asio::ip::tcp::socket& socket, const char* buffer, size_t length)
  {
    return (_write_in_progress = fc::asio::write_some(socket, buffer, length)).wait();
  }
  size_t tcp_socket::impl::writesome(boost::asio::ip::tcp::socket& socket, const std::shared_ptr<const char>& buffer, size_t length, size_t offset)
  {
    return (_write_in_progress = fc::asio::write_some(socket, buffer, length, offset)).wait();
  }


  void tcp_socket::open()
  {
    my->_sock.open(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0).protocol());
  }

  bool tcp_socket::is_open()const {
    return my->_sock.is_open();
  }

  tcp_socket::tcp_socket(){};

  tcp_socket::~tcp_socket() {}

  void tcp_socket::flush() {}
  void tcp_socket::close() {
    try {
        if( is_open() )
        { 
          my->_sock.close();
        }
    } FC_RETHROW_EXCEPTIONS( warn, "error closing tcp socket" );
  }

  bool tcp_socket::eof()const {
    return !my->_sock.is_open();
  }

  size_t tcp_socket::writesome(const char* buf, size_t len) 
  {
    return my->_io_hooks->writesome(my->_sock, buf, len);
  }

  size_t tcp_socket::writesome(const std::shared_ptr<const char>& buf, size_t len, size_t offset) 
  {
    return my->_io_hooks->writesome(my->_sock, buf, len, offset);
  }

  fc::ip::endpoint tcp_socket::remote_endpoint()const
  {
    try
    {
      auto rep = my->_sock.remote_endpoint();
      return  fc::ip::endpoint(rep.address().to_v4().to_ulong(), rep.port() );
    } 
    FC_RETHROW_EXCEPTIONS( warn, "error getting socket's remote endpoint" );
  }


  fc::ip::endpoint tcp_socket::local_endpoint() const
  {
    try
    {
      auto boost_local_endpoint = my->_sock.local_endpoint();
      return fc::ip::endpoint(boost_local_endpoint.address().to_v4().to_ulong(), boost_local_endpoint.port() );
    } 
    FC_RETHROW_EXCEPTIONS( warn, "error getting socket's local endpoint" );
  }

  size_t tcp_socket::readsome( char* buf, size_t len ) 
  {
    return my->_io_hooks->readsome(my->_sock, buf, len);
  }

  size_t tcp_socket::readsome( const std::shared_ptr<char>& buf, size_t len, size_t offset ) {
    return my->_io_hooks->readsome(my->_sock, buf, len, offset);
  }

  void tcp_socket::connect_to( const fc::ip::endpoint& remote_endpoint ) {
    fc::asio::tcp::connect(my->_sock, fc::asio::tcp::endpoint( boost::asio::ip::address_v4(remote_endpoint.get_address()), remote_endpoint.port() ) ); 
  }

  void tcp_socket::bind(const fc::ip::endpoint& local_endpoint)
  {
    try
    {
      my->_sock.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(local_endpoint.get_address()),
                                                                                local_endpoint.port()));
    }
    catch (const std::exception& except)
    {
      wlog("Exception binding outgoing connection to desired local endpoint ${endpoint}: ${what}",
           ("endpoint", local_endpoint)("what", except.what()));
      FC_THROW("error binding to ${endpoint}: ${what}", ("endpoint", local_endpoint)("what", except.what()));
    }
  }

  void tcp_socket::enable_keep_alives(const fc::microseconds& interval)
  {
    if (interval.count() > 0)
    {
      boost::asio::socket_base::keep_alive option(true);
      my->_sock.set_option(option);
#if defined _WIN32 || defined WIN32 || defined OS_WIN64 || defined _WIN64 || defined WIN64 || defined WINNT
      struct tcp_keepalive keepalive_settings;
      keepalive_settings.onoff = 1;
      keepalive_settings.keepalivetime = (ULONG)(interval.count() / fc::milliseconds(1).count());
      keepalive_settings.keepaliveinterval = (ULONG)(interval.count() / fc::milliseconds(1).count());

      DWORD dwBytesRet = 0;
      if (WSAIoctl(my->_sock.native_handle(), SIO_KEEPALIVE_VALS, &keepalive_settings, sizeof(keepalive_settings),
                   NULL, 0, &dwBytesRet, NULL, NULL) == SOCKET_ERROR)
        wlog("Error setting TCP keepalive values");
#elif !defined(__clang__) || (__clang_major__ >= 6)
      // This should work for modern Linuxes and for OSX >= Mountain Lion
      int timeout_sec = interval.count() / fc::seconds(1).count();
      if (setsockopt(my->_sock.native_handle(), IPPROTO_TCP,
      #if defined( __APPLE__ )
                     TCP_KEEPALIVE,
       #elif defined( __OpenBSD__ )
                     SO_KEEPALIVE,
       #else
                     TCP_KEEPIDLE, 
       #endif
                     (char*)&timeout_sec, sizeof(timeout_sec)) < 0)
        wlog("Error setting TCP keepalive idle time");
# if defined(__OpenBSD__)
        int name[4];
        name[0] = CTL_NET;
        name[1] = PF_INET;
        name[2] = IPPROTO_TCP;

        int value;
        size_t sz;

        // get tics per second
        name[3] = TCPCTL_SLOWHZ;
        if (sysctl(name, 4, &value, &sz, NULL, 0) == -1)
          wlog("Error setting TCP keepalive interval");

        // set interval
        value *= timeout_sec;
        name[3] = TCPCTL_KEEPINTVL;
        if (sysctl(name, 4, NULL, NULL, &value, sizeof(value)) == -1)
          wlog("Error setting TCP keepalive interval");
# elif !defined(__APPLE__) || defined(TCP_KEEPINTVL) // TCP_KEEPINTVL not defined before 10.9
      if (setsockopt(my->_sock.native_handle(), IPPROTO_TCP, TCP_KEEPINTVL,
                     (char*)&timeout_sec, sizeof(timeout_sec)) < 0)
        wlog("Error setting TCP keepalive interval");
# endif // (__OpenBSD__) or (!__APPLE__ || TCP_KEEPINTVL)
#endif // !WIN32
    }
    else
    {
      boost::asio::socket_base::keep_alive option(false);
      my->_sock.set_option(option);
    }
  }

  void tcp_socket::set_io_hooks(tcp_socket_io_hooks* new_hooks)
  {
    my->_io_hooks = new_hooks ? new_hooks : &*my;
  }

  void tcp_socket::set_reuse_address(bool enable /* = true */)
  {
    FC_ASSERT(my->_sock.is_open());
    boost::asio::socket_base::reuse_address option(enable);
    my->_sock.set_option(option);
    // Note:
    //   There was old code here that tried to set SO_REUSEPORT as well, but it has been removed,
    //   because it is not necessary at least in BitShares.
    //   If we really need it, we can add a new function to set it.
  }


  class tcp_server::impl {
    public:
      impl()
      :_accept( fc::asio::default_io_service() )
      {
        _accept.open(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0).protocol());
      }

      ~impl(){
        try {
          _accept.close();
        } 
        catch ( boost::system::system_error& )
        {
           wlog( "unexpected exception ${e}", ("e", fc::except_str()) );
        }
      }

      boost::asio::ip::tcp::acceptor _accept;
  };
  void tcp_server::close() {
    if( my && my->_accept.is_open() ) 
    {
        try {
          my->_accept.close();
        }
        catch ( boost::system::system_error& )
        {
           wlog( "unexpected exception ${e}", ("e", fc::except_str()) );
        }
    }
    delete my;
    my = nullptr;
  }
  tcp_server::tcp_server()
  :my(nullptr) {
  }
  tcp_server::~tcp_server() {
    delete my;
  }


  void tcp_server::accept( tcp_socket& s ) 
  {
    try
    {
      FC_ASSERT( my != nullptr );
      fc::asio::tcp::accept( my->_accept, s.my->_sock  ); 
    } FC_RETHROW_EXCEPTIONS( warn, "Unable to accept connection on socket." );
  }
  void tcp_server::set_reuse_address(bool enable /* = true */)
  {
    if( !my ) 
      my = new impl;
#if defined _WIN32 || defined WIN32 || defined OS_WIN64 || defined _WIN64 || defined WIN64 || defined WINNT
    // https://learn.microsoft.com/en-us/windows/win32/winsock/using-so-reuseaddr-and-so-exclusiveaddruse
    BOOL enabled = enable ? TRUE : FALSE;
    if( ::setsockopt(
          my->_accept.native_handle(),
          SOL_SOCKET,
          SO_EXCLUSIVEADDRUSE,
          reinterpret_cast<const char*>(&enabled),
          sizeof(enabled)) == SOCKET_ERROR ) {
      throw boost::system::system_error(
              boost::system::error_code(
                ::WSAGetLastError(),
                boost::asio::error::get_system_category()));
    }
#else
    boost::asio::ip::tcp::acceptor::reuse_address option(enable);
    my->_accept.set_option(option);
    // Note:
    //   There was old code here that tried to set SO_REUSEPORT as well, but it has been removed.
    //   If we really need it, we can add a new function to set it.
    //   SO_REUSEPORT allows multiple processes to bind to and listen on the same port - at least
    //   this behavior is unwanted in BitShares.
    //   BTW for Solaris perhaps we need to set SO_EXCLBIND.
    //   See https://stackoverflow.com/questions/14388706/how-do-so-reuseaddr-and-so-reuseport-differ
#endif
  }
  void tcp_server::listen( uint16_t port ) 
  {
    if( !my ) 
      my = new impl;
    try
    {
      my->_accept.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(), port));
      my->_accept.listen(256);
    } 
    FC_RETHROW_EXCEPTIONS(warn, "error listening on socket");
  }
  void tcp_server::listen( const fc::ip::endpoint& ep ) 
  {
    if( !my ) 
      my = new impl;
    try
    {
      my->_accept.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string((string)ep.get_address()), ep.port()));
      my->_accept.listen();
    } 
    FC_RETHROW_EXCEPTIONS(warn, "error listening on socket");
  }

  fc::ip::endpoint tcp_server::get_local_endpoint() const
  {
    FC_ASSERT( my != nullptr );
    return fc::ip::endpoint(my->_accept.local_endpoint().address().to_v4().to_ulong(), 
                            my->_accept.local_endpoint().port() );
  }

  uint16_t tcp_server::get_port()const
  {
    FC_ASSERT( my != nullptr );
    return my->_accept.local_endpoint().port();
  }



} // namespace fc 
