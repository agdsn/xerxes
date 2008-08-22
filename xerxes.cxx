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
#include "epoll.hxx"
#include <boost/utility.hpp>
#include <boost/any.hpp>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <vector>

enum sock_opt_types{
  TCP,
  UNIX
};

class SocketOption {
  public:

  SocketOption(std::string new_file)
    : type(UNIX), file(new_file)
  {
  }
  SocketOption(std::string new_hostname, std::string new_port)
    : type(TCP), hostname(new_hostname), port(new_port)
  {
  }

  int type;
  std::string file;
  std::string hostname;
  std::string port;
};


void validate(boost::any& v, 
              const std::vector<std::string>& values,
              SocketOption* target_type, int)
{
    //static boost::regex r("(tcp|unix):([\\d\\w_-/.]+)(:(\\d+))?");
    static boost::regex r("(tcp|unix):(([\\d\\w_-]|\\.|/)+)(:(\\d+))?");

    using namespace boost::program_options;
    using namespace std;

    // Make sure no previous assignment to 'a' was made.
    validators::check_first_occurrence(v);
    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    const std::string& s = validators::get_single_string(values);

    // Do regex match and convert the interesting part to 
    // int.
    boost::smatch match;
    if(regex_match(s, match, r)) 
      {
        cout << "1: " << match[1] << endl; // Type
	cout << "2: " << match[2] << endl; // File-/Hostname
	cout << "5: " << match[5] << endl; // Port
        if(match[1] == "tcp")
	  {
	    cout << "TCP" << endl;
	    v = boost::any(SocketOption(match[2], match[5]));
	  }
	else
	  {
	    cout << "UNIX" << endl;
	    v = boost::any(SocketOption(match[2]));
	  }
      } 
    else 
      {
        throw validation_error("invalid value");
      }        
}



int
main(int argc, char* argv[])
{
  namespace po = boost::program_options;
  using namespace std;
  using namespace xerxes;
  cout << "Hello, World!" << endl
       << "ich kanns auch lassen, hier `Hello, World!' zu schreiben..." 
       << endl;



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

if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
}

if (vm.count("src")) {
    SocketOption sock = vm["src"].as<SocketOption>();
    if(sock.type == TCP)
      {
        cout << "TCP Source is " << sock.hostname << "," << sock.port << ".\n";
      }
    else
      {
        cout << "UNIX Source is " << sock.file << ".\n";
      }
} else {
    cout << "Source was not set.\n";
}




exit(0);


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
	      //getaddrinfo("127.0.0.1", "25", &hints, &res);
              connect(*target, res->ai_addr, res->ai_addrlen);
	      freeaddrinfo(res);

	      sockets[target->fd] = target;

	      boost::shared_ptr<Socket> source(accept(lstn, 0, 0));

	      sockets[source->fd] = source;

              epoll.add(*source, *target);
	    }
	  else
	    {
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
