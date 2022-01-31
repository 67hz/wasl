#ifndef WASL_SOCKET_H
#define WASL_SOCKET_H

#include <wasl/Common.h>
#include <wasl/Types.h>

#include <gsl/pointers>
#include <gsl/string_span> // czstring
#include <iostream>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#else
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#ifndef WASL_LISTEN_BACKLOG
#define WASL_LISTEN_BACKLOG 50
#endif

namespace wasl
{
namespace ip
{
using path_type = gsl::czstring<>;

template <int SocketFamily>
struct socket_traits {
	static constexpr int value = SocketFamily;
	using addr_type = struct sockaddr_storage;
};

template <>
struct socket_traits<AF_INET> {
	static constexpr int value = AF_INET;
	using addr_type = struct sockaddr_in;
};

template <>
struct socket_traits<AF_INET6> {
	static constexpr int value = AF_INET6;
	using addr_type = struct sockaddr_in6;
};

#ifdef SYS_API_LINUX
template <>
struct socket_traits<AF_LOCAL> { // == AF_UNIX
	static constexpr int value = AF_LOCAL;
	using addr_type = struct sockaddr_un;
};
#endif // SYS_API_LINUX

namespace {

template <int v>
struct int2Type {
	using type = int2Type<v>;
	static constexpr int value = v;
};

/// Return underlying socket family given a socketfd.
/// \note see Stevens UNPp109
int sockfd_to_family(SOCKET sfd) {
	struct sockaddr_storage addr {};
	socklen_t socklen = sizeof (addr);
	if (getsockname(sfd, (sockaddr *)&addr, &socklen) < 0)
		return -1;
	return addr.ss_family;
}

/// Get a socket's address struct returned as a sockaddr_storage struct.
/// Caller should cast returned struct to protocol-specific struct,
/// e.g., struct sockaddr_in.
/// \return generic sockaddr_storage struct.
auto get_address (SOCKET sfd)
{
	struct sockaddr_storage res;
  memset (&res, 0, sizeof (res));
  socklen_t socklen = sizeof(res);

  getsockname (sfd, (SOCKADDR *)&res, &socklen);

  return res;
}

/// Connect a socket_node to a peer's socket descriptor
/// \param node SocketNode
/// \param link socket fd containing address to connect with
template <typename T, EnableIfSocketType<typename T::traits::addr_type> = true,
          socklen_t len = (sizeof (typename T::traits::addr_type))>
SOCKET
socket_connect (const T &node, SOCKET link)
{
  if (!node)
    {
      return INVALID_SOCKET;
    }

  auto srv_addr = get_address(link);
  return connect (sockno (node), (SOCKADDR *)&(srv_addr), sizeof (srv_addr));

}

template <typename T, EnableIfSocketType<typename T::traits::addr_type> = true>
SOCKET
socket_listen (const T &node)
{
  if (!is_valid_socket (sockno (node)))
    return INVALID_SOCKET;

  return ::listen (sockno (node), WASL_LISTEN_BACKLOG);
}

/// Accept a socket_node via a listening socket
/// \return fd of accepted socket
template <typename T, EnableIfSocketType<typename T::traits::addr_type> = true,
          socklen_t len = (sizeof (typename T::traits::addr_type))>
SOCKET
socket_accept (T *listening_node)
{
  if (!listening_node)
    {
      return INVALID_SOCKET;
    }

  auto socklen = len;
  typename T::traits::addr_type peer_addr;
  return accept (sockno (*listening_node), (SOCKADDR *)(&peer_addr), &socklen);
}

} // namespace anon





template <typename SocketNode, typename isTCP = void> struct socket_builder;

/// RAII Wrapper for a socket that close()'s socket connection on destruction.
/// \tparam Family socket family (address protocol suite)
/// \tparam SocketType  Type of socket, e.g., SOCK_DGRAM
/// \TODO check is_integral_type or use value param
template <int Family, int SocketType>
class socket_node
{
public:
	using traits = socket_traits<Family>;
	constexpr static int socket_type = SocketType;

  ~socket_node ()
  {
    if (is_valid_socket (sd))
      {
#ifndef NDEBUG
        std::cout << "closing socket: " << sd << '\n';
#endif
        closesocket (sd);
      }

#ifdef SYS_API_WIN32
    WSACleanup();
#endif

    // TODO check platform here
    // unlink(_addr->sun_path);
  }

	/// \TODO use isfdtype(int fd, int fdtype) to check actual socket existence
  inline friend constexpr bool
  is_open (const socket_node &node) noexcept
  {
    return is_valid_socket (node.sd);
  }

  inline friend constexpr SOCKET
  sockno (const socket_node &node) noexcept
  {
    return node.sd;
  }

  /// Get a pointer to the associated socket address struct
  inline friend constexpr std::unique_ptr<typename traits::addr_type>
  address (const socket_node &node) noexcept
  {
		auto addr = new sockaddr_storage;
		*addr = get_address(node.sd);
		return std::unique_ptr<typename traits::addr_type>(
				reinterpret_cast<typename traits::addr_type*>(addr));
  }

	/// \return socket_builder instance
  static auto
  create ()
  {
    return std::make_unique<socket_builder<socket_node<Family, SocketType>>>();
  }

private:
  friend socket_builder<socket_node<Family, SocketType>>;

#ifdef SYS_API_WIN32
  WSAData wsaData;
#endif


  SOCKET sd{ INVALID_SOCKET }; // a socket descriptor

  /// Construction is enforced through socket_builder to ensure valid
  /// initialization of sockets.
  socket_node () {
#ifdef SYS_API_WIN32
    std::cout << "winsock init" << '\n';
    auto iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    assert(iResult == 0);
#endif
  }
};




/// Primary socket builder builds TCP sockets
/// \tparam IsTCP defaults to void for non-datagrams
template <typename SocketNode, typename IsTCP> struct socket_builder
{
  static constexpr int socket_type = SocketNode::socket_type;
	using traits = typename SocketNode::traits;

	std::unique_ptr<SocketNode> sock;

  // indicates where errors occur in build pipeline, clients should check
  // errno with GET_SOCKERRNO() to handle
  SockError sock_err = SockError::ERR_NONE;

  socket_builder ();

  auto
  get_error ()
  {
#ifndef NDEBUG
    std::cerr << "wasl error: " << local::toUType (sock_err) << '\n';
#endif
    return GET_SOCKERRNO ();
  }

  socket_builder *socket ();

  socket_builder *bind (path_type service, path_type host = "");

  socket_builder *listen ();

  explicit operator bool () const
  {
    return !local::toUType (sock_err) && is_open (*sock);
  }

	/// \return new'd ptr. Caller has responsibility to wrap in owning ptr.
	std::unique_ptr<SocketNode>
  build ()
  {
    return std::move(sock);
  }

};

#ifdef SYS_API_LINUX
struct sockaddr_un create_dgram_address(path_type host, int3Type<AF_UNIX>) {
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	assert( (strlen(host) < sizeof(addr.sun_path) - 1));

	// remove path in case artifacts were left from a previous run
	unlink(host);

	strncpy(addr.sun_path, host, sizeof(addr.sun_path) - 1);
	return addr;
}
#endif

struct sockaddr_in create_dgram_address(path_type host, int2Type<AF_INET>) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	std::cout << "UDP datagrams work" << '\n';
	return addr;
}


// for datagrams
template <typename SocketNode>
struct socket_builder<SocketNode, typename std::enable_if_t<std::is_same<int2Type<SOCK_DGRAM>, int2Type<SocketNode::socket_type>>::value>> {
	using traits = typename SocketNode::traits;

	std::unique_ptr<SocketNode> sock;

  // indicates where errors occur in build pipeline, clients should check
  // errno with GET_SOCKERRNO() to handle
  SockError sock_err = SockError::ERR_NONE;

  socket_builder ()
		: sock { new SocketNode }
	{
	// tag dispatch based on socket family
		//create_dgram_address(sock, host, int2Type<traits::value>());
	}

  socket_builder *socket () {
    sock->sd = ::socket(traits::value, SocketNode::socket_type, 0);

    if (!is_valid_socket(sock->sd))
        sock_err |= SockError::ERR_SOCKET;

    return this;
	}

	socket_builder *bind (path_type host) {
	// tag dispatch based on socket family
		auto addr = create_dgram_address(host, int2Type<traits::value>());
    if (::bind(sockno(*sock), reinterpret_cast<struct sockaddr *>(&addr),
               sizeof(typename traits::addr_type)) == -1)
    {

#ifndef NDEBUG
//        std::cerr << "Bind error: " << strerror(GET_SOCKERRNO()) << '\n';
#endif
        sock_err |= SockError::ERR_BIND;
    }

    return this;
	}


	std::unique_ptr<SocketNode>
  build ()
  {
    return std::move(sock);
  }
};



/// makers for creating a unique_ptr to a socket_node
/// TODO add overloads for:
/// passive_opens (e.g., TCP servers) adding listen() to builder chain
/// active opens (e.g., client sockets) adding connect() to builder chain
/// see Stevens UNPp34
/// \tparam Family Any of struct sockaddr_x socket types e.g.: { sockaddr_un,
/// sockaddr_in, ...}
/// \tparam SocketType socket type used in socket() call
/// e.g.: {SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...}
/// Unix domain Datagram sockets maker
template <int Family, int SocketType,
          std::enable_if_t<std::is_same<typename socket_traits<Family>::addr_type, struct sockaddr_un>::value,
                           bool> = true >
auto
make_socket (path_type sock_path)
{
  auto socket{ socket_node<Family, SocketType>::create ()
                   ->socket ()
                   ->bind (sock_path)
                   ->build () };

#ifndef NDEBUG
  if (!socket)
    {
     // std::cerr << strerror (GET_SOCKERRNO ()) << '\n';
    }
#endif

  return std::unique_ptr<socket_node<Family, SocketType> > (
      std::move (socket));
}

/// TCP sockets maker
template <int Family, int SocketType,
          std::enable_if_t<std::is_same<typename socket_traits<Family>::addr_type, struct sockaddr_in>::value,
                           bool> = true >
auto
make_socket (path_type service, path_type sock_path)
{
  auto socket{ socket_node<Family, SocketType>::create ()
                   ->socket ()
                   ->bind (service, sock_path)
                   ->listen ()
                   ->build () };

#ifndef NDEBUG
  if (!socket)
    {
     // std::cerr << strerror (GET_SOCKERRNO ()) << '\n';
    }
#endif

  return std::unique_ptr<socket_node<Family, SocketType> > (
      std::move (socket));
}

} // namespace ip
} // namespace wasl

#endif /* WASL_SOCKET_H */
