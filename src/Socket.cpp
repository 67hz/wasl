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
        sock->sock_err |= SockError::ERR_SOCKET;

		int optval {1};
		if (setsockopt(sock->sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
			sock->sock_err |= SockError::ERR_SOCKET_OPT;

    return this;
}

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::bind(path_type service, path_type host)
{
	typename traits::addr_type* addr = create_address(service, host, int2Type<traits::value>());

    if (::bind(sockno(*sock), reinterpret_cast<struct sockaddr *>(addr),
               sizeof(typename traits::addr_type)) == -1)
    {

#ifndef NDEBUG
//        std::cerr << "Bind error: " << strerror(GET_SOCKERRNO()) << '\n';
#endif
        sock->sock_err |= SockError::ERR_BIND;
    }

    return this;
}

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::listen()
{
    if (socket_listen(*sock) == INVALID_SOCKET)
    {
        sock->sock_err |= SockError::ERR_LISTEN;
    }
    return this;
}

#ifdef SYS_API_LINUX
template struct socket_builder<socket_node<AF_UNIX, SOCK_DGRAM>>;
#endif

template struct socket_builder<socket_node<AF_INET , SOCK_STREAM>>;

} // namespace ip
} // namespace wasl
