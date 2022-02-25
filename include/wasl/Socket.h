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

namespace wasl {
namespace ip {
using path_type = gsl::czstring<>;

struct Address_info {
  path_type host{ nullptr };
  path_type service{ nullptr };
  bool reuse_addr{true};
};

template <typename SocketNode> struct socket_builder;
template <typename Derived> struct socket_builder_base;
template <int Family, int SocketType> class wasl_socket;

// \NOTE sockaddr_in.sa_family_t holds family in struct but for compile-time
// these traits are useful.
template <int SocketFamily> struct socket_traits {
  static constexpr int value = SocketFamily;
  using addr_type = struct sockaddr_storage;
};

template <> struct socket_traits<AF_INET> {
  static constexpr int value = AF_INET;
  using addr_type = struct sockaddr_in;
};

template <> struct socket_traits<AF_INET6> {
  static constexpr int value = AF_INET6;
  using addr_type = struct sockaddr_in6;
};

#ifdef SYS_API_LINUX
template <> struct socket_traits<AF_LOCAL> { // == AF_UNIX
  static constexpr int value = AF_LOCAL;
  using addr_type = struct sockaddr_un;
};
#endif // SYS_API_LINUX

namespace {

/// Return underlying socket family given a socketfd.
/// \note see Stevens UNPp109
int sockfd_to_family(SOCKET sfd) {
  struct sockaddr_storage addr {};
  socklen_t socklen = sizeof(addr);
  if (getsockname(sfd, (sockaddr *)&addr, &socklen) < 0)
    return -1;
  return addr.ss_family;
}

/// Get a socket's address struct returned as a sockaddr_storage struct.
/// Caller should cast returned struct to protocol-specific struct,
/// e.g., struct sockaddr_in.
/// \return generic sockaddr_storage struct.
struct sockaddr_storage get_address(SOCKET sfd) {
  struct sockaddr_storage res;
  memset(&res, 0, sizeof(res));
  socklen_t socklen = sizeof(res);

  getsockname(sfd, (SOCKADDR *)&res, &socklen);

  return res;
}

/// create AF_INET, AF_INET6 as DGRAMS or SOCK_STREAM
template <int Family, int SocketType, typename traits = socket_traits<Family>>
auto create_inet_address(Address_info info) {
  struct addrinfo hints;
  struct addrinfo *result;
  int rc;

  memset(&hints, 0, sizeof(hints));
  // hints.ai_family = AF_INET;
  hints.ai_family = Family;
  // hints.ai_socktype = SOCK_STREAM;
  hints.ai_socktype = SocketType;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV; // for wildcard IP address
  // hints.ai_protocol = IPPROTO_TCP;
  hints.ai_protocol = 0; // TODO allow override

  rc = getaddrinfo(info.host, info.service, &hints, &result);
  if (rc != 0) {
    /// \TODO handle error
    std::cout << "getaddrinfo failed: " << gai_strerror(rc) << '\n';
  }

#if 0
		if (rc != 0) {
        sock->sock_err |= SockError::ERR_BIND;
				return this;
		}
#endif
  auto addr = reinterpret_cast<typename traits::addr_type *>(result->ai_addr);
  //		freeaddrinfo(result);
  return addr;
}

template <int Family, int SocketType, typename traits = socket_traits<Family>>
typename traits::addr_type *
create_address(Address_info info,
               std::integral_constant<int, AF_INET>) {
  return create_inet_address<Family, SocketType>(info);
}

template <int Family, int SocketType, typename traits = socket_traits<Family>>
typename traits::addr_type *
create_address(Address_info info,
               std::integral_constant<int, AF_INET6>) {
  return create_inet_address<Family, SocketType>(info);
}

#ifdef SYS_API_LINUX
template <int Family, int SocketType, typename traits = socket_traits<Family>>
typename traits::addr_type *
create_address(Address_info info,
               std::integral_constant<int, AF_UNIX>) {
  struct sockaddr_un *addr = new sockaddr_un;
  addr->sun_family = AF_UNIX;
  assert((strlen(info.host) < sizeof(addr->sun_path) - 1));

  // remove path in case artifacts were left from a previous run
  unlink(info.host);

  strncpy(addr->sun_path, info.host, sizeof(addr->sun_path) - 1);
  return addr;
}
#endif

/// Connect a wasl_socket to a peer's socket descriptor
/// \param node SocketNode
/// \param link socket fd containing address to connect with
template <int Family, int Socket_Type,
          EnableIfSocketType<typename socket_traits<Family>::addr_type> = true,
          socklen_t len = sizeof(typename socket_traits<Family>::addr_type)>
SOCKET socket_connect(const wasl_socket<Family, Socket_Type> &node,
                      SOCKET link) {
  if (!is_valid_socket(sockno(node))) {
    return INVALID_SOCKET;
  }

  auto srv_addr = get_address(link);
  return connect(sockno(node), (SOCKADDR *)&(srv_addr), len);
}

template <int Family, int Socket_Type,
          EnableIfSocketType<typename socket_traits<Family>::addr_type> = true>
SOCKET socket_listen(const wasl_socket<Family, Socket_Type> &node) {
  if (!is_valid_socket(sockno(node)))
    return INVALID_SOCKET;

  return ::listen(sockno(node), WASL_LISTEN_BACKLOG);
}

/// Accept a wasl_socket via a listening socket
/// \return fd of accepted socket
template <int Family, int Socket_Type,
          typename traits = typename wasl_socket<Family, Socket_Type>::traits,
          EnableIfSocketType<typename traits::addr_type> = true,
          socklen_t len = (sizeof(typename traits::addr_type))>
SOCKET socket_accept(const wasl_socket<Family, Socket_Type> *listening_node) {
  if (!listening_node) {
    return INVALID_SOCKET;
  }

  auto socklen = len;
  typename traits::addr_type peer_addr;
  return accept(sockno(*listening_node), (SOCKADDR *)(&peer_addr), &socklen);
}

} // namespace

/// RAII Wrapper for a socket that close()'s socket connection on destruction.
/// \tparam Family socket family (address protocol suite)
/// \tparam SocketType  Type of socket, e.g., SOCK_DGRAM
/// \TODO check is_integral_type or use value param
template <int Family, int SocketType> class wasl_socket {
public:
  using traits = socket_traits<Family>;
  constexpr static int socket_type = SocketType;

  // indicates where errors occur in build pipeline, clients should check
  // errno with GET_SOCKERRNO() to handle
  SockError sock_err = SockError::ERR_NONE;

#ifdef SYS_API_LINUX
  void destroy(std::integral_constant<int, AF_UNIX>) {
    auto addr{as_address(*this)};
    if (addr->sun_path)
      remove(addr->sun_path);
  }
#endif

  // close socket for everything except unix domain sockets
  void destroy(...) {
    if (is_valid_socket(sd)) {
#ifndef NDEBUG
      std::cout << "closing socket: " << sd << '\n';
#endif
      closesocket(sd);
    }
  }

  ~wasl_socket() {
    destroy(std::integral_constant<int, traits::value>());

#ifdef SYS_API_WIN32
    WSACleanup();
#endif
  }

  auto get_error() const {
#ifndef NDEBUG
    std::cerr << "wasl error: " << local::toUType(sock_err) << '\n';
#endif
    return GET_SOCKERRNO();
  }

  /// \TODO use isfdtype(int fd, int fdtype) to check actual socket existence
  inline friend constexpr bool is_open(const wasl_socket &node) noexcept {
    return is_valid_socket(node.sd);
  }

  inline friend constexpr SOCKET sockno(const wasl_socket &node) noexcept {
    return node.sd;
  }

  /// Get a pointer to the associated socket address struct
  inline friend constexpr std::unique_ptr<typename traits::addr_type>
  as_address(const wasl_socket &node) noexcept {
    auto addr = new sockaddr_storage;
    *addr = get_address(node.sd);
    return std::unique_ptr<typename traits::addr_type>(
        reinterpret_cast<typename traits::addr_type *>(addr));
  }

  /// \return socket_builder instance
  static auto create() {
    return std::make_unique<socket_builder<wasl_socket<Family, SocketType>>>();
  }

private:
  SOCKET sd{INVALID_SOCKET}; // a socket descriptor

  friend socket_builder<wasl_socket<Family, SocketType>>;

  template <typename B> friend struct socket_builder_base;

#ifdef SYS_API_WIN32
  WSAData wsaData;
#endif

  /// Construction is enforced through socket_builder to ensure valid
  /// initialization of sockets.
  wasl_socket() {
#ifdef SYS_API_WIN32
    auto iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    assert(iResult == 0);
#endif
  }
};

template <typename Derived> struct socket_builder_base {

  // make alias type dependent on Derived to avoid compiler throwing incomplete
  // struct error
  // see accessing child types in c++ using crtp
  template <typename D = Derived> using traits = typename D::traits;

  template <typename D = Derived> using node = typename D::node;

  socket_builder_base() = default;

#if 0
	Derived& asDerived() { return *static_cast<Derived*>(this); }
	Derived const& asDerived() const { return *static_cast<Derived const*>(this); }
#endif

  Derived *asDerived() { return static_cast<Derived *>(this); }

  Derived *socket() {
    asDerived()->sock->sd = ::socket(Derived::family, Derived::socket_type, 0);

    if (!is_valid_socket(asDerived()->sock->sd))
      asDerived()->sock->sock_err |= SockError::ERR_SOCKET;

    return asDerived();
  }

  Derived *connect(Address_info info) {
    auto addr = asDerived()->make_address(info);
    std::cout << "fd: " << sockno(*(asDerived()->sock)) << '\n';
    if (::connect(sockno(*(asDerived()->sock)),
                  reinterpret_cast<struct sockaddr *>(addr),
                  sizeof(typename traits<Derived>::addr_type)) ==
        INVALID_SOCKET) {

      //		if (socket_connect( asDerived()->sock, info) == -1) {
      std::cout << "connect error: " << strerror(errno) << '\n';
      asDerived()->sock->sock_err |= SockError::ERR_CONNECT;
    }
    return asDerived();
  }

  Derived *bind(Address_info info) {
    // TODO create partial specialization or tag dispatch these?
    auto addr = asDerived()->make_address(info);

    if (info.reuse_addr) {
      int optval{1};
      if (setsockopt(asDerived()->sock->sd, SOL_SOCKET, SO_REUSEADDR,
                     (const char *)&optval, sizeof(optval)) == -1)
        asDerived()->sock->sock_err |= SockError::ERR_SOCKET_OPT;
    }

    if (::bind(sockno(*(asDerived()->sock)),
               reinterpret_cast<struct sockaddr *>(addr),
               sizeof(typename traits<Derived>::addr_type)) == -1) {

#ifndef NDEBUG
//        std::cerr << "Bind error: " << strerror(GET_SOCKERRNO()) << '\n';
#endif
      asDerived()->sock->sock_err |= SockError::ERR_BIND;
    }

    return asDerived();
  }
};

/// Primary socket builder builds TCP sockets
/// \tparam IsTCP defaults to void for non-datagrams
template <typename SocketNode>
struct socket_builder : public socket_builder_base<socket_builder<SocketNode>> {
  using traits = typename SocketNode::traits;
  static constexpr int socket_type = SocketNode::socket_type;
  static constexpr int family = traits::value;
  using node = SocketNode;

  std::unique_ptr<SocketNode> sock;

  socket_builder() : sock{new SocketNode} {}

  auto make_address(Address_info info) {
    auto addr = create_address<family, socket_type>(
        info, std::integral_constant<int, traits::value>());
    return addr;
  }

  // TODO reenable in partially specialized inet (TCP) builder
  // socket_builder *listen ();

  explicit operator bool() const {
    return !local::toUType(sock->sock_err) && is_open(*sock);
  }

  std::unique_ptr<SocketNode> build() { return std::move(sock); }
};

/// Make a unique_ptr to a wasl_socket
/// TODO add overloads for:
/// passive_opens (e.g., TCP servers) adding listen() to builder chain
/// active opens (e.g., client sockets) adding connect() to builder chain
/// see Stevens UNPp34
/// \tparam Family Any of AF_INET, AF_INET6, SOCKADDR_UN
/// \tparam SocketType socket type used in socket() call
/// e.g.: {SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...}
/// inet sockets maker
template <int Family, int SocketType,
          EnableIfSocketType<typename socket_traits<Family>::addr_type> = true>
std::unique_ptr<wasl_socket<Family, SocketType>>
make_socket(Address_info info) {
  auto socket{
      wasl_socket<Family, SocketType>::create()->socket()->bind(info)->build()};

#ifndef NDEBUG
  if (!socket) {
    // std::cerr << strerror (GET_SOCKERRNO ()) << '\n';
  }
#endif

  return std::unique_ptr<wasl_socket<Family, SocketType>>(std::move(socket));
}

} // namespace ip
} // namespace wasl

#endif /* WASL_SOCKET_H */
