#include <wasl/Socket.h>

namespace wasl {
namespace ip {

template <typename Node>
socket_builder<Node>::socket_builder(typename sock_traits::path_type sock_path)
    : sock{gsl::owner<node_type *>(new node_type)} {
  sock->_addr.sun_family = sock_traits::domain;
  if (strlen(sock_path) > sizeof(sock->_addr.sun_path) - 1) {
    sock_err |= SockError::ERR_PATH_INVAL;
  }

  // remove path in case artifacts were left from a previous run
  unlink(sock_path);

  strncpy(sock->_addr.sun_path, sock_path, sizeof(sock->_addr.sun_path) - 1);
}

template <typename Node> socket_builder<Node> *socket_builder<Node>::socket() {
  sock->sd = ::socket(sock_traits::domain, socket_type, 0);

  if (!is_valid_socket(sock->sd))
    sock_err |= SockError::ERR_SOCKET;

  return this;
}

template <typename Node> socket_builder<Node> *socket_builder<Node>::bind() {
  if (::bind(sockno(*sock), reinterpret_cast<struct sockaddr *>(&(sock->_addr)),
             sizeof(typename sock_traits::type)) == -1) {

#ifndef NDEBUG
    std::cerr << "Bind error: " << strerror(GET_SOCKERRNO()) << '\n';
#endif
    sock_err |= SockError::ERR_BIND;
  }

  return this;
}

template <typename Node>
socket_builder<Node> *socket_builder<Node>::connect(SOCKET target_sd) {
  if (socket_connect(sock, target_sd) == -1) {
    sock_err |= SockError::ERR_CONNECT;
  }
  return this;
}

template <typename Node>
socket_builder<Node> *socket_builder<Node>::listen(SOCKET target_sd) {
}

#ifdef SYS_API_LINUX
template struct socket_builder<socket_node<struct sockaddr_un, SOCK_DGRAM>>;
#endif

template struct socket_builder<socket_node<struct sockaddr_in, SOCK_STREAM>>;

} // namespace ip
} // namespace wasl
