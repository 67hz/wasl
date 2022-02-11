#if 0
#include <wasl/Socket.h>

namespace wasl
{
namespace ip
{

#ifdef SYS_API_LINUX
template struct socket_builder<wasl_socket<AF_UNIX, SOCK_DGRAM>>;
#endif

template struct socket_builder<wasl_socket<AF_INET , SOCK_STREAM>>;

} // namespace ip
} // namespace wasl
#endif
