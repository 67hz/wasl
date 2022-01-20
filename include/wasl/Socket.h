#ifndef WASL_SOCKET_H
#define WASL_SOCKET_H

#include <wasl/Common.h>
#include <wasl/Types.h>

#include <gsl/pointers>
#include <gsl/string_span> // czstring
#include <type_traits>
#include <utility>
#include <iostream>

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

template <typename T> struct socket_traits {};

template <> struct socket_traits<struct sockaddr_in> {
  using type = struct sockaddr_in;
  using path_type = gsl::czstring<>;
  static constexpr int domain = AF_INET;
};

template <> struct socket_traits<struct sockaddr_in6> {
  using type = struct sockaddr_in6;
  using path_type = gsl::czstring<>;
  static constexpr int domain = AF_INET6;
};

#ifdef SYS_API_LINUX
template <> struct socket_traits<struct sockaddr_un> {
  using type = struct sockaddr_un;
  using path_type = gsl::czstring<>;
  static constexpr int domain = AF_LOCAL;
};
#endif

template <typename SocketNode> struct socket_builder;

template <typename AddrType, int Type,
          typename SockTraits = socket_traits<AddrType>>
class socket_node {
public:
  using type = socket_node<AddrType, Type, SockTraits>;
  using traits = socket_traits<AddrType>;
  using addr_type = AddrType;
  using path_type = typename traits::path_type;
  static constexpr int socket_type = Type;

  ~socket_node() {
    if (is_valid_socket(sd)) {
#ifndef NDEBUG
      std::cout << "closing socket: " << sd << '\n';
#endif
      closesocket(sd);
    }

    // TODO check platform here
    unlink(_addr->sun_path);
  }

  inline friend constexpr bool is_open(const socket_node &node) noexcept {
    return is_valid_socket(node.sd);
  }

  inline friend constexpr SOCKET sockno(const socket_node &node) noexcept {
    return node.sd;
  }

  /// Return the underlying socket address struct
  inline friend constexpr addr_type* c_addr(socket_node *node) noexcept {
    return node->_addr.get();
  }

  static auto create(path_type spath) {
    return std::make_unique<socket_builder<type>>(spath);
  }

private:
  friend socket_builder<type>;

	std::unique_ptr<addr_type> _addr {std::make_unique<addr_type>()};           // the underlying socket struct

  SOCKET sd{INVALID_SOCKET}; // a socket descriptor

  /// Construction is enforced through socket_builder to ensure valid
  /// initialization of sockets.
  socket_node() = default;
};

/// Get a socket's address struct
/// \tparam AddrType struct type of socket address
/// \return socket address struct, e.g. struct sockaddr_in
template <typename AddrType, EnableIfSocketType<AddrType> = true>
AddrType get_address(SOCKET sfd) {
  AddrType res;
  socklen_t socklen = sizeof(res);
  memset(&res, 0, sizeof(res));

  getsockname(sfd, (SOCKADDR *)&res, &socklen);

  return res;
}

/// Connect a socket_node to a peer's socket descriptor
template <typename T, EnableIfSocketType<typename T::addr_type> = true,
          socklen_t len = (sizeof(typename T::addr_type))>
int socket_connect(const T *node, SOCKET link) {
  if (!node) {
    return INVALID_SOCKET;
  }

  auto addr = get_address<typename T::addr_type>(link);
  return connect(sockno(*node), (SOCKADDR *)&(addr), sizeof(addr));
}

template <typename SocketNode> struct socket_builder {
public:
  static constexpr int socket_type = SocketNode::socket_type;
  using node_type = SocketNode;
  using sock_traits = socket_traits<typename SocketNode::addr_type>;

  gsl::owner<node_type *> sock;

  // indicates where errors occur in build pipeline, clients should check
  // errno with GET_SOCKERRNO() to handle
	SockError sock_err = SockError::ERR_NONE;

	explicit socket_builder(typename sock_traits::path_type socket_path);

  auto get_error() {
#ifndef NDEBUG
    std::cerr << "wasl error: " << local::toUType(sock_err) << '\n';
#endif
    return GET_SOCKERRNO();
  }

  socket_builder *socket();

  socket_builder *bind();

  socket_builder *connect(SOCKET target_sd);

  socket_builder *listen(SOCKET target_sd);

  explicit operator bool() const {
    return !local::toUType(sock_err) && is_open(*sock);
  }

  explicit operator gsl::owner<node_type *>() { return sock; }

  /// \note Built types outlive their builders so caller is responsible for
  /// deleting this returned pointer.
  gsl::owner<node_type *> build() { return sock; }
};

/// create a unique_ptr to a socket_node
/// TODO add overloads for:
/// passive_opens (e.g., TCP servers) adding listen() to builder chain
/// active opens (e.g., client sockets) adding connect() to builder chain
/// see Stevens UNPp34
/// \tparam AddrType Any of struct sockaddr_x socket types e.g.: { sockaddr_un,
/// sockaddr_in, ...}
/// \tparam SockType type of socket used in socket() call
/// e.g.: {SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...}
template <typename AddrType, int SockType,
          typename sock_traits = socket_traits<AddrType>>
auto make_socket(typename sock_traits::path_type sock_path) {
  auto socket{socket_node<AddrType, SockType>::create(sock_path)
                  ->socket()
                  ->bind()
                  ->build()};

  if (!socket) {
    std::cerr << strerror(GET_SOCKERRNO()) << '\n';
  }

  return std::unique_ptr<socket_node<AddrType, SockType>>(std::move(socket));
}

} // namespace ip
} // namespace wasl

#endif /* WASL_SOCKET_H */
