#include <wasl/Socket.h>

namespace wasl
{
namespace ip
{


#if 0
template <typename Node> socket_builder<Node, IsTCP> *socket_builder<Node, IsTCP>::listen()
{
    if (socket_listen(*sock) == INVALID_SOCKET)
    {
        sock->sock_err |= SockError::ERR_LISTEN;
    }
    return this;
}
#endif

#ifdef SYS_API_LINUX
template struct socket_builder<wasl_socket<AF_UNIX, SOCK_DGRAM>>;
#endif

template struct socket_builder<wasl_socket<AF_INET , SOCK_STREAM>>;

} // namespace ip
} // namespace wasl
