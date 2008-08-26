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
#include <signal.h>
#include "socket.hxx"
#include "epoll.hxx"
#include <boost/utility.hpp>
#include <vector>


int be_quiet = 0;
int be_debug = 0;

int
main(int argc, char* argv[])
{
  namespace po = boost::program_options;
  using namespace std;
  using namespace xerxes;

  signal(SIGPIPE, SIG_IGN);

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("quiet", "be quiet")
    ("debug", "write a lot of stupid debug messages")
    ("src", po::value<SocketOption>(), "Source")
    ("dst", po::value<SocketOption>(), "Destination")
  ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);    

  if (vm.count("help")) 
    {
      cout << desc << "\n";
      return 1;
    }

  if (vm.count("quiet"))
    {
      be_quiet = 1;
    }

  if (vm.count("debug"))
    {
      be_debug = 1;
    }

  if (vm.count("src")) 
    {
      SocketOption sock = vm["src"].as<SocketOption>();
    }

  if (vm.count("dst")) 
    {
      SocketOption sock = vm["dst"].as<SocketOption>();
      if(!be_quiet)
        {
          if(sock.type == TCP)
            {
              cout << "TCP Destination is " << sock.hostname << "," << sock.port << ".\n";
	    }
	  else
	    {
	      cout << "UNIX Destination is " << sock.file << ".\n";
	    }
	}
      } 
    else 
      {
        cout << "Destination was not set.\n";
        exit(0);
      }

  SocketOption src = vm["src"].as<SocketOption>();
  Socket &lstn =  *(src.gen_socket());

  if(src.type == TCP)
    {
      if(!be_quiet)
        {
          cout << "TCP Source is " << src.hostname << "," << src.port << ".\n";
	}
      bind_inet(lstn, src);
    }
  else
    {
      if(!be_quiet)
        {
          cout << "UNIX Source is " << src.file << ".\n";
	}
      bind_unix(lstn, src);
    }

  listen(lstn, 3);

  EPoll epoll;
  epoll.add(lstn);

  const int max_events = 23;
  boost::shared_array<epoll_event> events(new epoll_event[max_events]);
  std::map<int, boost::shared_ptr<Socket> > sockets;

  MysqlData buffer = makeData(1024);

  for(;;)
    {
      int num = epoll_wait(epoll.fd, events.get(), max_events, -1);

      if(be_debug)
        {
	  cout << "epoll events available (" << num << ")" << endl;
	}

      for(int i = 0; i < num; ++i)
	{
	  if(be_debug)
	    {
	      cout << "process event " << i << endl;
	    }
	  if((events[i].data.fd == -1) 
	     && (events[i].events & EPOLLIN))
	    {
	      if(be_debug)
	        {
                  cout << "EPOLLIN on listening socket .. create a new connection" << endl;
		}
              boost::shared_ptr<Socket> target;
              boost::shared_ptr<Socket> source;
	      try
	        {
	          SocketOption dst = vm["dst"].as<SocketOption>();
                  target = boost::shared_ptr<Socket>(dst.gen_socket());
	          if (dst.type == TCP)
	            {
                      connect_inet(*target, dst);
	            }
                  else
	            {
		      connect_unix(*target, dst);
	            }
	          sockets[target->fd] = target;
		}
	      catch (SocketErr)
	        {
                  continue;
		}

	      try 
	        {
	          source = boost::shared_ptr<Socket>(accept(lstn, 0, 0));
	          sockets[source->fd] = source;
                } 
	      catch (SocketErr e)
	        {
	          sockets.erase(target->fd);
                  continue;
	        }

              try
	        {
                   epoll.add(*source, *target);
	        }
	      catch (EpollAddErr e)
	        {
		  if (e.type != EPOLL_ADD_ERR_SOURCE)
		    {
		      epoll.del(*target);
		    }
		  epoll.del(*target);
                  epoll.del(*source);
                  sockets.erase(target->fd);
                  sockets.erase(source->fd);
                  continue;
		}
		if(be_debug)
		  {
		    cout << "new connection created" << endl;
		  }
	    }
	  else
	    {
	      if(sockets[events[i].data.fd] == 0) 
	        {
		  if(be_debug)
		    {
		      cout << "fd " << events[i].data.fd << " already closed, ignore event " << i << endl;
		    }
		  continue;
		}

	      boost::shared_ptr<Socket> target(sockets[events[i].data.fd]);
	      boost::shared_ptr<Socket> source(sockets[epoll.events[target->fd]->data.fd]);
	      
	      if((events[i].events & EPOLLIN )
	         || (events[i].events & EPOLLPRI))
		{
		  if(be_debug)
		    {
		      cout << "EPOLLIN or EPOLLPRI event from fd " << source->fd << " target is fd " << target->fd << endl;
		    }
		  try
		    {
                      int len = recv(*source, buffer, 0);
		      send(*target, buffer, len, 0);
                      
		    }
		  catch (SocketErr e)
		    {
		      if(be_debug)
		        {
			  cout << "Socket Error, closing socket" <<  target->fd << " and " << source->fd << endl;
			}
		      epoll.del(*target);
		      epoll.del(*source);
		      sockets.erase(target->fd);
		      sockets.erase(source->fd);
                      continue;
		    }
		}
	      if((events[i].events & EPOLLERR)
	         || (events[i].events & EPOLLHUP))
		{
		  if(be_debug)
                   {
                      cout << "EPOLLERR or EPOLLHUP event from fd " << source->fd << " target is fd " << target->fd << " close both" << endl;
                   }
		  epoll.del(*target);
		  epoll.del(*source);
		  sockets.erase(target->fd);
		  sockets.erase(source->fd);
                  continue;
		}
	    }
         }
      }
  return 0;
}
