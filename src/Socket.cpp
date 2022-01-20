#include <wasl/Socket.h>

#ifndef WASL_LISTEN_BACKLOG
#define WASL_LISTEN_BACKLOG 50
#endif

namespace wasl
{
namespace ip
{

namespace {

	// Unix sockers
	template <typename Node, std::enable_if_t<std::is_same<struct sockaddr_un, typename Node::addr_type>::value, bool> = true>
	int create_address(Node *node, typename Node::path_type socket_path) {
			c_addr(node)->sun_family = Node::traits::domain;
			if (strlen(socket_path) > sizeof(c_addr(node)->sun_path) - 1)
			{
				return INVALID_SOCKET;
			}

			// remove path in case artifacts were left from a previous run
			unlink(socket_path);

			strncpy(c_addr(node)->sun_path, socket_path, sizeof(c_addr(node)->sun_path) - 1);
			return 0;
	}

	// Streams-based sockets
	template <typename Node, std::enable_if_t<std::is_same<struct sockaddr_in, typename Node::addr_type>::value, bool> = true>
	int create_address(Node *node, typename Node::path_type socket_path) {
		std::cout << "test";
			c_addr(node)->sin_family = Node::traits::domain;
			c_addr(node)->sin_addr.s_addr = htonl(INADDR_ANY);
			c_addr(node)->sin_port = htons(9877);
			return 0;
	}
} // namespace anon

template <typename Node> socket_builder<Node>::socket_builder(typename sock_traits::path_type socket_path)
	: sock { gsl::owner<node_type *>(new node_type)} {
		create_address(sock, socket_path);
	}

template <typename Node> socket_builder<Node> *socket_builder<Node>::socket()
{
    sock->sd = ::socket(sock_traits::domain, socket_type, 0);

    if (!is_valid_socket(sock->sd))
        sock_err |= SockError::ERR_SOCKET;

    return this;
}

template <typename Node> socket_builder<Node> *socket_builder<Node>::bind()
{
    if (::bind(sockno(*sock), reinterpret_cast<struct sockaddr *>(c_addr(sock)),
               sizeof(typename sock_traits::type)) == -1)
    {

#ifndef NDEBUG
        std::cerr << "Bind error: " << strerror(GET_SOCKERRNO()) << '\n';
#endif
        sock_err |= SockError::ERR_BIND;
    }

    return this;
}

template <typename Node> socket_builder<Node> *socket_builder<Node>::connect(SOCKET target_sd)
{
    if (socket_connect(sock, target_sd) == INVALID_SOCKET)
    {
        sock_err |= SockError::ERR_CONNECT;
    }
    return this;
}

template <typename Node> socket_builder<Node> *socket_builder<Node>::listen(SOCKET target_sd)
{
    if (::listen(target_sd, WASL_LISTEN_BACKLOG) == INVALID_SOCKET)
    {
        sock_err |= SockError::ERR_LISTEN;
    }
    return this;
}

#ifdef SYS_API_LINUX
template struct socket_builder<socket_node<struct sockaddr_un, SOCK_DGRAM>>;
#endif

template struct socket_builder<socket_node<struct sockaddr_in, SOCK_STREAM>>;

} // namespace ip
} // namespace wasl
