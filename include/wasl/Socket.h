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
};

template <>
struct socket_traits<AF_INET> {
	static constexpr int value = AF_INET;
	using type = struct sockaddr_in;
};

template <>
struct socket_traits<AF_INET6> {
	static constexpr int value = AF_INET6;
	using type = struct sockaddr_in6;
};

template <>
struct socket_traits<AF_LOCAL> { // == AF_UNIX
	static constexpr int value = AF_LOCAL;
	using type = struct sockaddr_un;
};

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

/// Get a socket's address struct
/// \tparam AddrType struct type of socket address
/// \return socket address struct, e.g. struct sockaddr_in
auto get_address (SOCKET sfd)
{
	auto family = sockfd_to_family(sfd);
	struct sockaddr_storage res;
  memset (&res, 0, sizeof (res));
  socklen_t socklen;

	switch( family) {
		case AF_INET:
			socklen = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			socklen = sizeof(struct sockaddr_in6);
			break;
		case AF_UNIX:
			socklen = sizeof(struct sockaddr_un);
			break;
		default:
			socklen = sizeof(struct sockaddr_storage);
	}

  getsockname (sfd, (SOCKADDR *)&res, &socklen);

  return res;
}

/// Connect a socket_node to a peer's socket descriptor
/// \param node SocketNode
/// \param link socket fd containing address to connect with
template <typename T, EnableIfSocketType<typename T::traits::type> = true,
          socklen_t len = (sizeof (typename T::traits::type))>
int
socket_connect (const T *node, SOCKET link)
{
  if (!node)
    {
      return INVALID_SOCKET;
    }

  auto srv_addr = get_address(link);
  return connect (sockno (*node), (SOCKADDR *)&(srv_addr), sizeof (srv_addr));

}

template <typename T, EnableIfSocketType<typename T::traits::type> = true>
int
socket_listen (const T *node)
{
  if (!is_valid_socket (sockno (*node)))
    return INVALID_SOCKET;

  return ::listen (sockno (*node), WASL_LISTEN_BACKLOG);
}

/// Accept a socket_node via a listening socket
/// \return fd of accepted socket
template <typename T, EnableIfSocketType<typename T::traits::type> = true,
          socklen_t len = (sizeof (typename T::traits::type))>
SOCKET
socket_accept (T *listening_node)
{
  if (!listening_node)
    {
      return INVALID_SOCKET;
    }

  auto socklen = len;
  typename T::traits::type peer_addr;
  return accept (sockno (*listening_node), (SOCKADDR *)(&peer_addr), &socklen);
}

} // namespace anon





template <typename SocketNode, typename isTCP = void> struct socket_builder;

/// RAII Wrapper for a socket owning the socket's address struct and
/// close()'s socket connection on destruction.
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

  /// Returns a pointer to the underlying socket address struct
  /// \note If the socket is bound, getsockname syscall will also return the
	/// associated address struct.
  inline friend constexpr typename traits::type*
  c_addr (socket_node *node) noexcept
  {
    return node->_addr.get ();
  }

	/// \return socket_builder instance
  static auto
  create (int port)
  {
    return std::make_unique<socket_builder<socket_node<Family, SocketType>>>(port);
  }


  static auto
  create (path_type spath)
  {
    return std::make_unique<socket_builder<socket_node<Family, SocketType>>>(spath);
  }

  static auto
  create (int port, path_type spath)
  {
    return std::make_unique<socket_builder<socket_node<Family, SocketType>>>(port, spath);
  }


private:
  friend socket_builder<socket_node<Family, SocketType>>;

  /// \TODO use getsockname(sd) instead of holding _addr
	/// sockaddr.sa_family holds uint type e.g. AF_INET
	/// \note see man sys_socket.h for sockaddr_storage. this could be used to
	/// store a generic address struct and remove the family template param.
  std::unique_ptr<typename traits::type> _addr{
    std::make_unique<typename traits::type> ()
  }; // the underlying socket struct

  SOCKET sd{ INVALID_SOCKET }; // a socket descriptor

  /// Construction is enforced through socket_builder to ensure valid
  /// initialization of sockets.
  socket_node () = default;
};




/// Primary socket builder builds TCP sockets
/// \tparam IsTCP defaults to void for non-datagrams
template <typename SocketNode, typename IsTCP> struct socket_builder
{
public:
  static constexpr int socket_type = SocketNode::socket_type;
	using traits = typename SocketNode::traits;

	SocketNode* sock;

  // indicates where errors occur in build pipeline, clients should check
  // errno with GET_SOCKERRNO() to handle
  SockError sock_err = SockError::ERR_NONE;

	// TCP sockets require a port
  explicit socket_builder (path_type socket_path) = delete;

  socket_builder (int port, path_type socket_path = "");

  auto
  get_error ()
  {
#ifndef NDEBUG
    std::cerr << "wasl error: " << local::toUType (sock_err) << '\n';
#endif
    return GET_SOCKERRNO ();
  }

  socket_builder *socket ();

  socket_builder *bind ();

  socket_builder *connect (SOCKET target_sd);

  socket_builder *listen ();

  explicit operator bool () const
  {
    return !local::toUType (sock_err) && is_open (*sock);
  }

//  explicit operator family& () { return sock; }

	/// \return new'd ptr. Caller has responsibility to wrap in owning ptr.
	SocketNode*
  build ()
  {
    return sock;
  }
};

template <typename SocketNode>
int create_dgram_address(SocketNode sock, path_type socket_path, int2Type<AF_UNIX>) {
	c_addr(sock)->sun_family = AF_UNIX;
	if (strlen(socket_path) > sizeof(c_addr(sock)->sun_path) - 1)
	{
		return INVALID_SOCKET;
	}

	// remove path in case artifacts were left from a previous run
	unlink(socket_path);

	strncpy(c_addr(sock)->sun_path, socket_path, sizeof(c_addr(sock)->sun_path) - 1);
	return 0;
}

template <typename SocketNode>
int create_dgram_address(SocketNode sock, path_type socket_path, int2Type<AF_INET>) {
	std::cout << "UDP datagrams work" << '\n';
	return 0;
}


// for datagrams
template <typename SocketNode>
struct socket_builder<SocketNode, typename std::enable_if_t<std::is_same<int2Type<SOCK_DGRAM>, int2Type<SocketNode::socket_type>>::value>> {
	using traits = typename SocketNode::traits;

	SocketNode* sock;

  // indicates where errors occur in build pipeline, clients should check
  // errno with GET_SOCKERRNO() to handle
  SockError sock_err = SockError::ERR_NONE;
  explicit socket_builder () = default;
  explicit socket_builder (int port) {}

  explicit socket_builder (path_type socket_path)
		: sock { new SocketNode }
	{
	// tag dispatch based on socket family
		create_dgram_address(sock, socket_path, int2Type<traits::value>());
	}

  socket_builder *socket () {
    sock->sd = ::socket(traits::value, SocketNode::socket_type, 0);

    if (!is_valid_socket(sock->sd))
        sock_err |= SockError::ERR_SOCKET;

    return this;
	}

	socket_builder *bind () {
    if (::bind(sockno(*sock), reinterpret_cast<struct sockaddr *>(c_addr(sock)),
               sizeof(typename traits::type)) == -1)
    {

#ifndef NDEBUG
        std::cerr << "Bind error: " << strerror(GET_SOCKERRNO()) << '\n';
#endif
        sock_err |= SockError::ERR_BIND;
    }

    return this;
	}


	SocketNode*
  build ()
  {
    return sock;
  }
};



/// create a unique_ptr to a socket_node
/// TODO add overloads for:
/// passive_opens (e.g., TCP servers) adding listen() to builder chain
/// active opens (e.g., client sockets) adding connect() to builder chain
/// see Stevens UNPp34
/// \tparam Family Any of struct sockaddr_x socket types e.g.: { sockaddr_un,
/// sockaddr_in, ...}
/// \tparam SocketType socket type used in socket() call
/// e.g.: {SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...}
template <int Family, int SocketType,
          std::enable_if_t<std::is_same<typename socket_traits<Family>::type, struct sockaddr_un>::value,
                           bool> = true >
auto
make_socket (path_type sock_path)
{
  auto socket{ socket_node<Family, SocketType>::create (sock_path)
                   ->socket ()
                   ->bind ()
                   ->build () };

  if (!socket)
    {
      std::cerr << strerror (GET_SOCKERRNO ()) << '\n';
    }

  return std::unique_ptr<socket_node<Family, SocketType> > (
      std::move (socket));
}

template <int Family, int SocketType,
          std::enable_if_t<std::is_same<typename socket_traits<Family>::type, struct sockaddr_in>::value,
                           bool> = true >
auto
make_socket (int port, path_type sock_path)
{
  auto socket{ socket_node<Family, SocketType>::create (port, sock_path)
                   ->socket ()
                   ->bind ()
                   ->listen ()
                   ->build () };

  if (!socket)
    {
      std::cerr << strerror (GET_SOCKERRNO ()) << '\n';
    }

  return std::unique_ptr<socket_node<Family, SocketType> > (
      std::move (socket));
}

} // namespace ip
} // namespace wasl

#endif /* WASL_SOCKET_H */
