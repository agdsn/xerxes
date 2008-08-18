/**
 * xerxes - mysql proxying
 * ``Why do you persist in your loneliness?'' --Xerxes
 * (c) 2008 
 * Jan Losinski <losinshi@wh2.tu-dresden.de> 
 * Maximilian Marx <mmarx@wh2.tu-dresden.de>
 */

#ifndef XERXES_SOCKET_HXX
#define XERXES_SOCKET_HXX

#include <utility>
#include <boost/utility.hpp>
#include <boost/shared_array.hpp>
#include <sys/types.h>
#include <sys/socket.h>

namespace xerxes
{
  struct Socket : boost::noncopyable
  {
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
  
  int 
  accept(Socket& socket, 
	 sockaddr* address,
	 socklen_t* address_len);

  int 
  listen(Socket& socket, int backlog);

  int
  connect(Socket& socket,
	  sockaddr const* const serv_address,
	  socklen_t* address_len);

  int
  recv(Socket& socket,
       MysqlData& data,
       int flags);

  int
  send(Socket& socket,
       MysqlData& data,
       int flags);

  int
  bind(Socket& socket,
       sockaddr const* const bind_address,
       socklen_t addrlen);

}

#endif
