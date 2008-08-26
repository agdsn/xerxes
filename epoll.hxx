/**
 * xerxes - mysql proxying
 * ``Why do you persist in your loneliness?'' --Xerxes
 * (c) 2008 
 * Jan Losinski <losinshi@wh2.tu-dresden.de> 
 * Maximilian Marx <mmarx@wh2.tu-dresden.de>
 */

#ifndef XERXES_EPOLL_HXX
#define XERXES_EPOLL_HXX

#include <stdexcept>
#include <utility>
#include <map>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <sys/types.h>
#include <sys/epoll.h>
#include "socket.hxx"

namespace xerxes
{
  struct EPoll
  {
    EPoll();
    
    virtual ~EPoll();

    typedef boost::shared_ptr<epoll_event> event_t;

    void
    add(Socket const& source,
	Socket const& target); 

    void
    add(Socket const& socket);

    void
    del(Socket const& socket);

    int fd;
    std::map<int, event_t> events;
  };

  enum epoll_add_err_type
  {
    EPOLL_ADD_ERR_SINGLE,
    EPOLL_ADD_ERR_SOURCE,
    EPOLL_ADD_ERR_TARGET
  };

  class EpollErr
  {
    public:
    EpollErr();
    EpollErr(std::string err);
  };
  class EpollAddErr : public EpollErr {
    public:
    EpollAddErr();
    EpollAddErr(epoll_add_err_type t);
    epoll_add_err_type type;
  };
  class EpollDelErr : public EpollErr {
    public:
    EpollDelErr() : EpollErr("could not delete fd from epoll"){};
  };
  class EpollCreateErr : public EpollErr {
    public:
    EpollCreateErr() : EpollErr("could not create Epoll"){};
  };
}
#endif
