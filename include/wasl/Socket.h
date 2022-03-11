#ifndef WASL_SOCKET_H
#define WASL_SOCKET_H

#include <cstdint>
#include <ostream>
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

namespace wasl {
namespace ip {

using path_type = gsl::czstring<>;
static constexpr int WASL_LISTEN_BACKLOG{50};

struct Address_info {
  path_type host{nullptr};
  path_type service{nullptr};
  bool reuse_addr{true};
};

template <typename SocketNode> struct socket_builder;
template <typename Derived> class socket_builder_base;
template <int Family, int SocketType> class wasl_socket;

struct inet_socket_tag {};
struct path_socket_tag {};
struct AF_UNIX_tag : public path_socket_tag {};
struct AF_INET_tag : public inet_socket_tag {};
struct AF_INET6_tag : public inet_socket_tag {};

// \NOTE sockaddr_in.sa_family_t holds family in struct but for compile-time
// these traits are useful.
template <int SocketFamily> struct socket_traits {
  static constexpr int value = SocketFamily;
  using addr_type = struct sockaddr_storage;
  using tag = inet_socket_tag;
};

template <> struct socket_traits<AF_INET> {
  static constexpr int value = AF_INET;
  using addr_type = struct sockaddr_in;
  using tag = inet_socket_tag;
};

template <> struct socket_traits<AF_INET6> {
  static constexpr int value = AF_INET6;
  using addr_type = struct sockaddr_in6;
  using tag = inet_socket_tag;
};

#ifdef SYS_API_LINUX
template <> struct socket_traits<AF_LOCAL> { // == AF_UNIX
  static constexpr int value = AF_LOCAL;
  using addr_type = struct sockaddr_un;
  using tag = path_socket_tag;
};
#endif // SYS_API_LINUX

namespace {

/// Return underlying socket family given a socketfd.
/// \note see Stevens UNPp109
int sockfd_to_family(SOCKET sfd) {
  struct sockaddr_storage addr {};
  socklen_t socklen = sizeof(addr);
  if (getsockname(sfd, reinterpret_cast<SOCKADDR *>(&addr), &socklen) < 0)
    return -1;
  return addr.ss_family;
}

/// Get a socket's address struct returned as a sockaddr_storage struct.
/// Caller should cast returned struct to protocol-specific struct,
/// e.g., struct sockaddr_in.
/// \return generic sockaddr_storage struct.
struct sockaddr_storage get_address(SOCKET sfd) {
  struct sockaddr_storage res {};
  socklen_t socklen = sizeof(res);

  getsockname(sfd, reinterpret_cast<SOCKADDR *>(&res), &socklen);

  return res;
}

template <int Family, int SocketType, typename traits = socket_traits<Family>,
          typename Addr = typename traits::addr_type>
int create_address(Address_info info, Addr &out_addr, inet_socket_tag) {
  struct addrinfo hints {};
  struct addrinfo *result{nullptr};
  int rc = 0;

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
    return INVALID_SOCKET;
  }

  // TODO scan all results
  out_addr =
      *(reinterpret_cast<typename traits::addr_type *>(result[0].ai_addr));
  freeaddrinfo(result);
  return 0;
}

#ifdef SYS_API_LINUX
template <int Family, int SocketType, typename traits = socket_traits<Family>,
          typename Addr = typename traits::addr_type>
int create_address(Address_info info, Addr &out_addr, path_socket_tag) {
  out_addr.sun_family = AF_UNIX;
  assert((strlen(info.host) < sizeof(out_addr.sun_path) - 1));

  strncpy(out_addr.sun_path, info.host, sizeof(out_addr.sun_path) - 1);
  return 0;
}
#endif

/// Connect a wasl_socket to a peer's socket descriptor
/// \param node SocketNode
/// \param link socket fd containing address to connect with
template <int Family, int Socket_Type,
          EnableIfSocketType<typename socket_traits<Family>::addr_type> = true,
          socklen_t len = sizeof(typename socket_traits<Family>::addr_type)>
SOCKET socket_connect(const wasl_socket<Family, Socket_Type> &node,
                      const SOCKET link) {
  if (!is_valid_socket(sockno(node))) {
    return INVALID_SOCKET;
  }

  auto srv_addr = get_address(link);
  return connect(sockno(node), reinterpret_cast<SOCKADDR *>(&srv_addr), len);
}

template <int Family, int Socket_Type, typename traits = socket_traits<Family>,
          EnableIfSocketType<typename traits::addr_type> = true,
          socklen_t len = sizeof(typename traits::addr_type)>
SOCKET socket_connect(const wasl_socket<Family, Socket_Type> &node,
                      const Address_info info) {
  typename traits::addr_type peer_addr;
  if (create_address<Family, Socket_Type>(info, peer_addr,
                                          typename traits::tag()) == -1)
    return INVALID_SOCKET;

  return (::connect(sockno(node),
                    reinterpret_cast<struct sockaddr *>(&peer_addr),
                    sizeof(typename traits::addr_type)));
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
  typename traits::addr_type peer_addr{};
  return accept(sockno(*listening_node), (SOCKADDR *)(&peer_addr), &socklen);
}

} // namespace anon

/// RAII Wrapper for a socket that close()'s socket connection on destruction.
/// \tparam Family socket family (address protocol suite)
/// \tparam SocketType  Type of socket, e.g., SOCK_DGRAM
template <int Family, int SocketType> class wasl_socket {
public:
  using traits = socket_traits<Family>;
  constexpr static int socket_type = SocketType;

  wasl_socket() {
#ifdef SYS_API_WIN32
    auto iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    assert(iResult == 0);
#endif

    sd = ::socket(Family, SocketType, 0);

    if (!is_valid_socket(sd))
      sock_err |= SockError::ERR_SOCKET;
  }

  wasl_socket &bind(Address_info info) {
    typename traits::addr_type addr{};
    if (create_address<Family, SocketType>(info, addr,
                                           typename traits::tag()) == -1)
      sock_err |= SockError::ERR_BIND;

    if (Family == AF_LOCAL) { // unix domain
      // remove any artifacts from socket invocation
      remove(info.host);
    }

    if (info.reuse_addr) {
      int optval{1};
      if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) ==
          -1)
        sock_err |= SockError::ERR_SOCKET_OPT;
    }

    if (::bind(sd, reinterpret_cast<struct sockaddr *>(&addr),
               sizeof(typename traits::addr_type)) == -1) {
      sock_err |= SockError::ERR_BIND;
    }
    return *this;
  }

  wasl_socket &connect(Address_info peer_info) {
    if (socket_connect(*this, peer_info) == -1) {
      sock_err |= SockError::ERR_CONNECT;
    }
    return *this;
  }

  void close() { this->destroy(typename traits::tag()); }

  ~wasl_socket() {
    destroy(typename traits::tag());

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
    gsl::owner<sockaddr_storage *> addr = new sockaddr_storage;
    *addr = get_address(node.sd);
    return std::unique_ptr<typename traits::addr_type>(
        reinterpret_cast<typename traits::addr_type *>(addr));
  }

  explicit operator bool() const {
    return !local::toUType(sock_err) && is_open(*this);
  }

  inline friend constexpr bool operator==(const wasl_socket &lhs,
                                          const wasl_socket &rhs) {
    return lhs.sd == rhs.sd;
  }

  inline friend constexpr bool operator!=(const wasl_socket &lhs,
                                          const wasl_socket &rhs) {
    return !(lhs.sd == rhs.sd);
  }

  inline constexpr SockError error() const { return sock_err; }

private:
  SockError sock_err{SockError::ERR_NONE};
  SOCKET sd{INVALID_SOCKET}; // a socket descriptor
#ifdef SYS_API_WIN32
  WSAData wsaData;
#endif

  friend socket_builder<wasl_socket<Family, SocketType>>;

  template <typename B> friend class socket_builder_base;

#ifdef SYS_API_LINUX
  void destroy(path_socket_tag) {
#ifndef NDEBUG
    std::cout << "\n\nunix destructor\n";
#endif
    auto addr{as_address(*this)};
    if (addr->sun_path)
      remove(addr->sun_path);
  }
#endif

  // close socket for everything except unix domain sockets
  void destroy(inet_socket_tag) {
    if (is_valid_socket(sd)) {
#ifndef NDEBUG
      std::cout << "closing socket: " << sd << '\n';
#endif
      closesocket(sd);
    }
  }
};

/// Make a unique_ptr to a wasl_socket
/// TODO add overloads for:
/// passive_opens (e.g., TCP servers) adding listen() to builder chain
/// active opens (e.g., client sockets) adding connect() to builder chain
/// see Stevens UNPp34
/// \tparam Family Any of AF_INET, AF_INET6, SOCKADDR_UN
/// \tparam SocketType socket type used in socket() call
/// e.g.: {SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...}
template <int Family, int SocketType,
          EnableIfSocketType<typename socket_traits<Family>::addr_type> = true>
std::unique_ptr<wasl_socket<Family, SocketType>>
make_socket(Address_info info) {
  static_assert((SocketType & (SOCK_STREAM | SOCK_DGRAM | SOCK_RAW)) != 0,
                "invalid socket type");
  auto sock{std::make_unique<wasl_socket<Family, SocketType>>()};
  sock->bind(info);

#ifndef NDEBUG
  if (!sock) {
    // std::cerr << strerror (GET_SOCKERRNO ()) << '\n';
  }
#endif

  return std::move(sock);
}

template <int Family, int SocketType,
          EnableIfSocketType<typename socket_traits<Family>::addr_type> = true>
std::unique_ptr<wasl_socket<Family, SocketType>>
make_socket_listener(Address_info info) {
  auto sock = make_socket<Family, SocketType>(info);
  socket_listen(*sock);
  return std::move(sock);
}

} // namespace ip
} // namespace wasl

#endif /* WASL_SOCKET_H */
