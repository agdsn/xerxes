/**
 * xerxes - mysql proxying
 * ``Why do you persist in your loneliness?'' --Xerxes
 * (c) 2008 
 * Jan Losinski <losinshi@wh2.tu-dresden.de> 
 * Maximilian Marx <mmarx@wh2.tu-dresden.de>
 */

#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "socket.hxx"
#include <iostream>

namespace xerxes
{
  Socket::Socket(int protocol,
		int type,
		int domain)
    : fd(socket(protocol, type, domain))
  {
    std::cerr << "new socket" << std::endl;
    if(fd < 0)
      {
        perror("--");
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

  SocketOption::SocketOption(std::string new_file)
    : type(UNIX), file(new_file)
  {
  }

  SocketOption::SocketOption(std::string new_hostname, std::string new_port)
    : type(TCP), hostname(new_hostname), port(new_port)
  {
  }

  Socket*
  SocketOption::gen_socket()
  {
    if(type == TCP)
      {
        return new Socket(PF_INET, SOCK_STREAM, 0);
      }
    else
      {
        return new Socket(AF_UNIX, SOCK_STREAM, 0);
      }
   }


  void validate(boost::any& v, 
                const std::vector<std::string>& values,
                SocketOption* target_type, int)
  {
    static boost::regex r("(tcp|unix):(([\\d\\w_-]|\\.|/)+)(:(\\d+))?");

    using namespace boost::program_options;
    using namespace std;

    validators::check_first_occurrence(v);
    const std::string& s = validators::get_single_string(values);

    boost::smatch match;
    if(regex_match(s, match, r)) 
      {
        if(match[1] == "tcp")
	  {
	    v = boost::any(SocketOption(match[2], match[5]));
	  }
	else
	  {
	    v = boost::any(SocketOption(match[2]));
	  }
      } 
    else 
      {
        throw validation_error("invalid value");
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
  connect_inet(Socket& socket,
          SocketOption& opt)
  {
    addrinfo hints;
    addrinfo* res;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
    getaddrinfo(opt.hostname.c_str(), opt.port.c_str(), &hints, &res);
    int ret = connect(socket, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    return ret;
  }

  int
  connect_unix(Socket& socket,
            SocketOption& opt)
  {
    struct sockaddr_un adr;

    memset(&adr, 0, sizeof(adr));
    adr.sun_family = AF_UNIX;
    strncpy(adr.sun_path, opt.file.c_str(), sizeof(adr.sun_path));
    return connect(socket, (struct sockaddr *) &adr, SUN_LEN(&adr));
  }
  
  int
  recv(Socket& socket,
       MysqlData& data,
       int flags)
  {
    int ret = ::recv(socket.fd, data.first.get(), data.second, flags);
    if(ret == 0)
      {
        throw ConResetErr();
      }
    if(ret < 0)
      {
        throw ConDataErr();
      }
    return ret;
  }

  int
  send (Socket& socket,
	MysqlData& data,
	int len,
	int flags)
  {
    int ret = ::send(socket.fd, data.first.get(), len, flags);
    if(ret == 0)
      {
        throw ConResetErr();
      }
    if(ret < 0)
      {
        throw ConDataErr();
      }
    return ret;
  }

  int
  bind(Socket& socket,
       sockaddr const* const bind_address,
       socklen_t addrlen)
  {
    return ::bind(socket.fd, bind_address, addrlen);
  }

  int
  bind_inet(Socket& socket,
       SocketOption& opt)
  {
    addrinfo hints;
    addrinfo* res;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

    getaddrinfo(opt.hostname.c_str(), opt.port.c_str(), &hints, &res);

    int ret = bind(socket, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);
    return ret;
  }


  int
  bind_unix(Socket& socket,
       SocketOption& opt)
  {
    unlink(opt.file.c_str());
    
    struct sockaddr_un adr;

    memset(&adr, 0, sizeof(adr));
    adr.sun_family = AF_UNIX;
    strncpy(adr.sun_path, opt.file.c_str(), sizeof(adr.sun_path));
    return bind(socket, (struct sockaddr *) &adr, SUN_LEN(&adr));
  }
}
