/**
 * xerxes - mysql proxying
 * ``Why do you persist in your loneliness?'' --Xerxes
 * (c) 2008 
 * Jan Losinski <losinshi@wh2.tu-dresden.de> 
 * Maximilian Marx <mmarx@wh2.tu-dresden.de>
 */

#include "epoll.hxx"

namespace xerxes
{
  EPoll::EPoll() : fd(epoll_create(64))
  {
    if(fd < 0)
      {
        throw EpollCreateErr();
        //throw std::runtime_error("could not create epoll descriptor.");
      }
  }
  
  EPoll::~EPoll()
  {
    if(fd > 0)
      {
        close(fd);
      }
  }

  void
  EPoll::add(Socket const& source,
      Socket const& target)
  {
    events[source.fd] = event_t(new epoll_event);
    events[target.fd] = event_t(new epoll_event);
    events[source.fd]->events = EPOLLIN
    	| EPOLLPRI | EPOLLERR | EPOLLHUP;
    events[target.fd]->events = events[source.fd]->events;
    events[source.fd]->data.fd = target.fd;
    events[target.fd]->data.fd = source.fd;
    
    if(0 != epoll_ctl(fd, EPOLL_CTL_ADD, source.fd, events[source.fd].get()))
      throw EpollAddErr(EPOLL_ADD_ERR_SOURCE); 
    if(0 != epoll_ctl(fd, EPOLL_CTL_ADD, target.fd, events[target.fd].get()))
      throw EpollAddErr(EPOLL_ADD_ERR_TARGET);
  }

  void
  EPoll::add(Socket const& socket)
  {
    events[socket.fd] = event_t(new epoll_event);
    events[socket.fd]->events = EPOLLIN;
    events[socket.fd]->data.fd = -1;

    if(0 != epoll_ctl(fd, EPOLL_CTL_ADD, socket.fd, events[socket.fd].get()))
      throw EpollAddErr();
  }

  void
  EPoll::del(Socket const& socket)
  {
    if(0 != epoll_ctl(fd, EPOLL_CTL_DEL, socket.fd, 0))
      throw EpollAddErr();
    events.erase(socket.fd);
  }

  EpollErr::EpollErr()
  {
    EpollErr("unknown");
  }

  EpollErr::EpollErr(std::string err)
  {
    std::cerr << "Epoll Error: " << err << std::endl;
    perror("ERRNO");
  }
  EpollAddErr::EpollAddErr()
    {
      EpollAddErr(ADD_ERR_SINGLE);
    }
  EpollAddErr::EpollAddErr(epoll_add_err_type t)
    :EpollErr("could not add fd to epoll!"), type(t)
  {
    
  }

  
}
