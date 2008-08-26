/**
 * xerxes - mysql proxying
 * ``Why do you persist in your loneliness?'' --Xerxes
 * (c) 2008 
 * Jan Losinski <losinshi@wh2.tu-dresden.de> 
 * Maximilian Marx <mmarx@wh2.tu-dresden.de>
 */

#ifndef XERXES_SOCKET_HXX
#define XERXES_SOCKET_HXX

#include <stdexcept>
#include <utility>
#include <map>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/any.hpp>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>

namespace xerxes
{
  struct Socket : boost::noncopyable
  {
    explicit Socket(int fd);
    Socket(int domain,
	   int type,
	   int protocol);
    virtual ~Socket();
    
    int fd;

  private:
    Socket() {};
  };

  typedef boost::shared_array<char> buffer_t;
  typedef std::pair<buffer_t, int> MysqlData;

  MysqlData 
  makeData(int len);

  enum sock_opt_types
  {
    TCP,
    UNIX
  };

  class SocketOption 
  {
    public:

    SocketOption(std::string new_file);
    SocketOption(std::string new_hostname, std::string new_port);

    Socket* gen_socket();

    int type;
    std::string file;
    std::string hostname;
    std::string port;
  };

  void validate(boost::any& v, 
                const std::vector<std::string>& values,
                SocketOption* target_type, int);

  boost::shared_ptr<Socket>
  accept(Socket& socket, 
	 sockaddr* address,
	 socklen_t* address_len);

  int 
  listen(Socket& socket, int backlog);

  int
  connect(Socket& socket,
	  sockaddr const* const serv_address,
	  socklen_t address_len);

  int
  connect_inet(Socket& socket,
          SocketOption& opt);

  int
  connect_unix(Socket& socket,
          SocketOption& opt);

  int
  recv(Socket& socket,
       MysqlData& data,
       int flags);

  int
  send(Socket& socket,
       MysqlData& data,
       int len,
       int flags);

  int
  bind(Socket& socket,
       sockaddr const* const bind_address,
       socklen_t addrlen);

  int
  bind_inet(Socket& socket,
       SocketOption& opt);

  int
  bind_unix(Socket& socket,
       SocketOption& opt);

  class SocketErr{
    public:
    SocketErr(std::string err);
    SocketErr();
  };
  class ConResetErr: public SocketErr
  {
    public:
    ConResetErr() :SocketErr("Connection resetted."){};
  };
  class ConDataErr: public SocketErr
  {
    public:
    ConDataErr() :SocketErr("could send/recv Data."){};
  };
  class ConnCreateErr: public SocketErr 
  {
    public:
    ConnCreateErr() :SocketErr("could not create socket."){};
  };
  class ConnListenErr: public SocketErr 
  {
    public:
    ConnListenErr() :SocketErr("could not listen on socket."){};
  };
  class ConnAcceptErr: public SocketErr 
  {
    public:
    ConnAcceptErr() :SocketErr("could not accept on socket."){};
  };
  class ConnConnectErr: public SocketErr 
  {
    public:
    ConnConnectErr() :SocketErr("could not connect to socket."){};
  };
  class ConnBindErr: public SocketErr 
  {
    public:
    ConnBindErr() :SocketErr("could not bind on socket."){};
  };
}

#endif
