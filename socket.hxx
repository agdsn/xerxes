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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

namespace xerxes
{
  struct Socket : boost::noncopyable
  {
    Socket(int fd);
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


  class SocketErr{};
  class ConResetErr: public SocketErr{};
  class ConDataErr: public SocketErr{};
}

#endif
