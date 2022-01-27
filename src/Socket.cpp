#include <wasl/Socket.h>

namespace wasl
{
namespace ip
{

// TCP sockets
template <typename SocketNode, typename IsTCP>
socket_builder<SocketNode, IsTCP>::socket_builder()
	: sock { new SocketNode} { }

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::socket()
{
    sock->sd = ::socket(traits::value, socket_type, 0);

    if (!is_valid_socket(sock->sd))
        sock_err |= SockError::ERR_SOCKET;

    return this;
}

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::bind(int service, path_type host)
{
		typename traits::addr_type addr;
		addr.sin_family = traits::value;
		in_addr_t res_addr;

		if (strlen(host) &&
				inet_pton(traits::value, host, &res_addr) != 0) {
				addr.sin_addr.s_addr = res_addr;
		} else {
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
		}

		addr.sin_port = htons(service);

    if (::bind(sockno(*sock), reinterpret_cast<struct sockaddr *>(&addr),
               sizeof(typename traits::addr_type)) == -1)
    {

#ifndef NDEBUG
        std::cerr << "Bind error: " << strerror(GET_SOCKERRNO()) << '\n';
#endif
        sock_err |= SockError::ERR_BIND;
    }

    return this;
}

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::listen()
{
    if (socket_listen(*sock) == INVALID_SOCKET)
    {
        sock_err |= SockError::ERR_LISTEN;
    }
    return this;
}

#ifdef SYS_API_LINUX
template struct socket_builder<socket_node<AF_UNIX, SOCK_DGRAM>>;
#endif

template struct socket_builder<socket_node<AF_INET , SOCK_STREAM>>;

} // namespace ip
} // namespace wasl
