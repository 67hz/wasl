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

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::bind(path_type service, path_type host)
{
		struct addrinfo hints;
		struct addrinfo *result;
		int rc;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = Node::traits::value;
		hints.ai_socktype = Node::socket_type;
		hints.ai_flags = AI_PASSIVE; // for wildcard IP address
		hints.ai_protocol = IPPROTO_TCP;

		rc = getaddrinfo(host, service, &hints, &result);
		if (rc != 0) {
        sock_err |= SockError::ERR_BIND;
				return this;
		}

    if (::bind(sockno(*sock), reinterpret_cast<struct sockaddr *>(result->ai_addr),
               result->ai_addrlen) == -1)
    {

#ifndef NDEBUG
        std::cerr << "Bind error: " << strerror(GET_SOCKERRNO()) << '\n';
#endif
        sock_err |= SockError::ERR_BIND;
    }

		freeaddrinfo(result);
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
