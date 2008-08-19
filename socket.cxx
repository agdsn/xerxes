/**
 * xerxes - mysql proxying
 * ``Why do you persist in your loneliness?'' --Xerxes
 * (c) 2008 
 * Jan Losinski <losinshi@wh2.tu-dresden.de> 
 * Maximilian Marx <mmarx@wh2.tu-dresden.de>
 */

#include <unistd.h>
#include "socket.hxx"

namespace xerxes
{
 Socket::Socket(int protocol,
		int type,
		int domain)
   : fd(socket(protocol, type, domain))
  {
    if(fd < 0)
      {
	throw std::runtime_error("could not create socket.");
      }
  }

  Socket::Socket(int new_fd)
  {
    fd = new_fd;
  }

  Socket::~Socket()
  {
    if(fd)
      {
	close(fd);
      }
  }

  MysqlData 
  makeData(int len)
  {
    return MysqlData(buffer_t(new char[len]), len);
  }

  int
  listen(Socket& socket, int backlog)
  {
    return ::listen(socket.fd, backlog);
  }
  
  boost::shared_ptr<Socket>
  accept(Socket& socket, 
	 sockaddr* address,
	 socklen_t* address_len)
  {
     int new_fd = ::accept(socket.fd, address, address_len);
     if(new_fd == -1)
       {
	 throw std::runtime_error("could not accept socket.");
       }

     return boost::shared_ptr<Socket>(new Socket(new_fd));  
  }

  int
  connect(Socket& socket,
	  sockaddr const* const serv_addr,
	  socklen_t addrlen)
  {
    return ::connect(socket.fd, serv_addr, addrlen);
  }
  
  int
  recv(Socket& socket,
       MysqlData& data,
       int flags)
  {
    return ::recv(socket.fd, data.first.get(), data.second, flags);
  }

  int
  send (Socket& socket,
	MysqlData& data,
	int flags)
  {
    return ::send(socket.fd, data.first.get(), data.second, flags);
  }

  int
  bind(Socket& socket,
       sockaddr const* const bind_address,
       socklen_t addrlen)
  {
    return ::bind(socket.fd, bind_address, addrlen);
  }
}
