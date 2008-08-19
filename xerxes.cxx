/**
 * xerxes - mysql proxying
 * ``Why do you persist in your loneliness?'' --Xerxes
 * (c) 2008 
 * Jan Losinski <losinshi@wh2.tu-dresden.de> 
 * Maximilian Marx <mmarx@wh2.tu-dresden.de>
 */	


#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "socket.hxx"

int
main(int argc, char* argv[])
{
  using namespace std;
  using namespace xerxes;
  cout << "Hello, World!" << endl
       << "ich kanns auch lassen, hier `Hello, World!' zu schreiben..." 
       << endl;

  Socket lstn(PF_INET, SOCK_STREAM, 0);

  addrinfo hints;
  addrinfo* res;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

  getaddrinfo("127.0.0.1", "13337", &hints, &res);

  bind(lstn, res->ai_addr, res->ai_addrlen);

  freeaddrinfo(res);

  int ret = listen(lstn, 3);

  EPoll epoll;
  epoll.add(lstn);

  const int max_events = 23;
  boost::shared_array<epoll_event> events(new epoll_event[max_events]);
  std::map<int, boost::shared_ptr<Socket> > sockets;

  for(;;)
    {
      int num = epoll_wait(epoll.fd, events.get(), max_events, -1);

      for(int i = 0; i < num; ++i)
	{
	  if((events[i].data.fd == -1) 
	     && (events[i].events & EPOLLIN))
	    {
	      //close(accept(lstn, 0, 0));
	      //cout << "hollo!" << endl;
              boost::shared_ptr<Socket> target(new Socket(PF_INET, SOCK_STREAM, 0));
	      addrinfo hints;
	      addrinfo* res;

	      memset(&hints, 0, sizeof(hints));

	      hints.ai_family = AF_INET;
	      hints.ai_socktype = SOCK_STREAM;
	      hints.ai_protocol = IPPROTO_TCP;
	      hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

	      getaddrinfo("127.0.0.1", "3306", &hints, &res);

              connect(*target, res->ai_addr, res->ai_addrlen);

	      freeaddrinfo(res);

	      sockets[target->fd] = target;

	      boost::shared_ptr<Socket> source(accept(lstn, 0, 0));

	      sockets[source->fd] = source;

              epoll.add(*source, *target);
	    }
	  else{ cout << "hollo!" << events[i].data.fd << endl; }
	}
    }

  return 0;
}
