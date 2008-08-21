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

  struct EPoll
  {
    EPoll() : fd(epoll_create(64))
    {
      if(fd < 0)
	{
	  throw std::runtime_error("could not create epoll descriptor.");
	}
    }
    
    virtual ~EPoll()
    {
      if(fd > 0)
	{
	  close(fd);
	}
    }

    typedef boost::shared_ptr<epoll_event> event_t;

    void
    add(Socket const& source,
	Socket const& target)
    {
      events[source.fd] = event_t(new epoll_event);
      events[target.fd] = event_t(new epoll_event);
      events[source.fd]->events = EPOLLIN
      	| EPOLLPRI | EPOLLERR | EPOLLHUP;
      events[target.fd]->events = events[source.fd]->events;
      events[source.fd]->data.fd = target.fd;
      events[target.fd]->data.fd = source.fd;
      
      epoll_ctl(fd, EPOLL_CTL_ADD, source.fd, events[source.fd].get());
      epoll_ctl(fd, EPOLL_CTL_ADD, target.fd, events[target.fd].get());
    }

    void
    add(Socket const& socket)
    {
      events[socket.fd] = event_t(new epoll_event);
      events[socket.fd]->events = EPOLLIN;
      events[socket.fd]->data.fd = -1;

      epoll_ctl(fd, EPOLL_CTL_ADD, socket.fd, events[socket.fd].get());
    }

    void
    del(Socket const& socket)
    {
      epoll_ctl(fd, EPOLL_CTL_DEL, socket.fd, 0);
      
      events.erase(socket.fd);
    }

    int fd;
    std::map<int, event_t> events;
  };

  class SocketErr{};
  class ConResetErr: public SocketErr{};
  class ConDataErr: public SocketErr{};
}

#endif
