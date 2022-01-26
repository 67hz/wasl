#include <wasl/Socket.h>



namespace wasl
{
namespace ip
{

namespace {

	// Unix domain sockets
	template <typename SocketNode, std::enable_if_t<std::is_same<struct sockaddr_un, typename SocketNode::traits::addr_type>::value, bool> = true>
	int create_address(SocketNode *node, path_type socket_path) {
			c_addr(node)->sun_family = SocketNode::traits::value;
			if (strlen(socket_path) > sizeof(c_addr(node)->sun_path) - 1)
			{
				return INVALID_SOCKET;
			}

			// remove path in case artifacts were left from a previous run
			unlink(socket_path);

			strncpy(c_addr(node)->sun_path, socket_path, sizeof(c_addr(node)->sun_path) - 1);
			return 0;
	}

} // namespace anon

// TCP sockets
template <typename SocketNode, typename IsTCP>
socket_builder<SocketNode, IsTCP>::socket_builder(int port, path_type socket_path)
	: sock { new SocketNode} {
			c_addr(sock)->sin_family = SocketNode::traits::value;
			in_addr_t addr;

			if (strlen(socket_path) &&
					inet_pton(SocketNode::traits::value, socket_path, &addr) != 0) {
					c_addr(sock)->sin_addr.s_addr = addr;
			} else {
				c_addr(sock)->sin_addr.s_addr = htonl(INADDR_ANY);
			}

			c_addr(sock)->sin_port = htons(port);
}

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::socket()
{
    sock->sd = ::socket(traits::value, socket_type, 0);

    if (!is_valid_socket(sock->sd))
        sock_err |= SockError::ERR_SOCKET;

    return this;
}

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::bind()
{
    if (::bind(sockno(*sock), reinterpret_cast<struct sockaddr *>(c_addr(sock)),
               sizeof(typename traits::addr_type)) == -1)
    {

#ifndef NDEBUG
        std::cerr << "Bind error: " << strerror(GET_SOCKERRNO()) << '\n';
#endif
        sock_err |= SockError::ERR_BIND;
    }

    return this;
}

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::connect(SOCKET target_sd)
{
    if (socket_connect(sock, target_sd) == INVALID_SOCKET)
    {
        sock_err |= SockError::ERR_CONNECT;
    }
    return this;
}

template <typename Node, typename IsTCP> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::listen()
{
    if (socket_listen(sock) == INVALID_SOCKET)
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
