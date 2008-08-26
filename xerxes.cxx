/**
 * xerxes - mysql proxying
 * ``Why do you persist in your loneliness?'' --Xerxes
 * (c) 2008 
 * Jan Losinski <losinshi@wh2.tu-dresden.de> 
 * Maximilian Marx <mmarx@wh2.tu-dresden.de>
 */	

/**
 * TODO : Unix Socket testing
 * TODO : Do some Error handling
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




int
main(int argc, char* argv[])
{
  namespace po = boost::program_options;
  using namespace std;
  using namespace xerxes;
  cout << "Hello, World!" << endl
       << "ich kanns auch lassen, hier `Hello, World!' zu schreiben..." 
       << endl;


  signal(SIGPIPE, SIG_IGN);

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
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


  if (vm.count("src")) 
    {
      SocketOption sock = vm["src"].as<SocketOption>();
    }
  if (vm.count("dst")) 
    {
      SocketOption sock = vm["dst"].as<SocketOption>();
      if(sock.type == TCP)
        {
          cout << "TCP Destination is " << sock.hostname << "," << sock.port << ".\n";
        }
      else
        {
          cout << "UNIX Destination is " << sock.file << ".\n";
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
      cout << "TCP Source is " << src.hostname << "," << src.port << ".\n";
      bind_inet(lstn, src);
    }
  else
    {
      cout << "UNIX Source is " << src.file << ".\n";
      bind_unix(lstn, src);
    }

  int ret = listen(lstn, 3);
  if( ret != 0)
    {
      exit(1);
    }

  EPoll epoll;
  epoll.add(lstn);

  const int max_events = 23;
  boost::shared_array<epoll_event> events(new epoll_event[max_events]);
  std::map<int, boost::shared_ptr<Socket> > sockets;

  MysqlData buffer = makeData(1024);

  for(;;)
    {
      int num = epoll_wait(epoll.fd, events.get(), max_events, -1);

      for(int i = 0; i < num; ++i)
	{
	  if((events[i].data.fd == -1) 
	     && (events[i].events & EPOLLIN))
	    {
	      SocketOption dst = vm["dst"].as<SocketOption>();
              boost::shared_ptr<Socket> target(dst.gen_socket());
              connect_inet(*target, dst);
	      sockets[target->fd] = target;

              cerr << "accept!" << endl;
	      boost::shared_ptr<Socket> source(accept(lstn, 0, 0));

	      sockets[source->fd] = source;

              epoll.add(*source, *target);
	    }
	  else
	    {
	      if(sockets[events[i].data.fd] == 0) 
	        {
		  cerr << "already closed, ignore" << endl;
		  continue;
		}
	      //lookup
	      boost::shared_ptr<Socket> target(sockets[events[i].data.fd]);
	      boost::shared_ptr<Socket> source(sockets[epoll.events[target->fd]->data.fd]);
	      if((events[i].events & EPOLLIN )
	         || (events[i].events & EPOLLPRI))
		{
		  // read -> write
		  cout << "writer: "<< source->fd << endl;
		  cout << "reader: "<< target->fd << endl;
		  try
		    {
                      int len = recv(*source, buffer, 0);
		      send(*target, buffer, len, 0);
                      
		    }
		  catch (SocketErr e)
		    {
		      // hangup
		      cout << "Socket Error, closing " <<  target->fd << " and " << source->fd << endl;
		      epoll.del(target->fd);
		      epoll.del(source->fd);
		      sockets.erase(target->fd);
		      sockets.erase(source->fd);
		      cerr << "closed" << endl;
                      continue;
		    }
		}
	      if((events[i].events & EPOLLERR)
	         || (events[i].events & EPOLLHUP))
		{
		  // hangup
		  cout << target->fd << " closed by " << source->fd << endl;
		  epoll.del(target->fd);
		  epoll.del(source->fd);
		  sockets.erase(target->fd);
		  sockets.erase(source->fd);
                  continue;
		}
	    }
         }
      }
  return 0;
}
