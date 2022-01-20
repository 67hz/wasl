#include <wasl/Socket.h>
#include <wasl/SockStream.h>
#include <string>

#include <type_traits>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace testing;

using namespace std::string_literals;
using namespace wasl::ip;
using wasl::local::toUType;


constexpr gsl::czstring<> srv_path{"/tmp/wasl/srv"};
constexpr gsl::czstring<> client_path{"/tmp/wasl/client"};

#ifdef SYS_API_LINUX
using socket_local = socket_node<struct sockaddr_un, SOCK_DGRAM>;

TEST(socket_builder, CanBuildUnixDomainDatagram) {
	std::unique_ptr<socket_local> sockUP { socket_local::
    create(srv_path)->socket()->bind()->build() };

	ASSERT_NE(sockno(*sockUP), INVALID_SOCKET);
	ASSERT_TRUE(sockUP);
}

TEST(socket_builder, MakeHelperUnixDomainDatagram) {
	auto sockUP { make_socket<sockaddr_un, SOCK_DGRAM>(srv_path) };
	ASSERT_NE(sockno(*sockUP), INVALID_SOCKET);
	ASSERT_TRUE(is_open(*sockUP));
}

TEST(socket, CanConnectToPeerOnSameHost) {
	auto s1 { make_socket<sockaddr_un, SOCK_DGRAM>(srv_path) };
	auto s2 { make_socket<sockaddr_un, SOCK_DGRAM>(client_path) };

}

#endif // unix domain tests

TEST(SockErrorFlags, LogicalOpsAreTrue) {
	auto err = SockError::ERR_SOCKET | SockError::ERR_CONNECT;
	ASSERT_FALSE(toUType(err & SockError::ERR_BIND) != 0);
	err |= SockError::ERR_BIND;
	ASSERT_TRUE(toUType(err & SockError::ERR_BIND) != 0);
}

// TODO MOVE to sockstream tests
#ifdef SYS_API_LINUX
TEST(datagram_sockets, CanReadAndWrite) {
	auto msg = "some_message"s;
	auto srvUP { make_socket<sockaddr_un, SOCK_DGRAM>(srv_path) };
	auto srv_fd { sockno(*srvUP) };
	sockstream ss_srv(srv_fd);

	fork_and_wait([ss = &ss_srv, msg] {
		// parent reads
		std::string res;
		*ss >> res;
		ASSERT_EQ(res, msg);
	}, [msg, srv_fd]() {
		// child writes
		auto client_sock { make_socket<sockaddr_un, SOCK_DGRAM>(client_path) };
		socket_connect(client_sock.get(), srv_fd);
		sockstream ss_cl(sockno(*client_sock));
		ss_cl << msg << std::endl;
	});
}
#endif
