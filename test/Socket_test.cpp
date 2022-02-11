#include <wasl/Socket.h>
#include <string>
#include <memory>

#include <type_traits>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace testing;

using namespace std::string_literals;
using namespace wasl::ip;
using wasl::local::toUType;

constexpr gsl::czstring<> srv_addr{HOST};
constexpr gsl::czstring<> srv_path{"/tmp/wasl/srv"};
constexpr gsl::czstring<> client_path{"/tmp/wasl/client"};

using socket_tcp = wasl_socket<AF_INET, SOCK_STREAM>;
using socket_udp = wasl_socket<AF_INET, SOCK_DGRAM>;
using socket_dgram = wasl_socket<AF_INET, SOCK_DGRAM>;

TEST(socket_utils, GetAddressFromSocketDescriptor) {
	std::unique_ptr<socket_tcp> sock { socket_tcp::
		create()->socket()->bind(SERVICE, srv_addr)->build() };
	auto addr = get_address(sockno(*sock));
	in_addr ip;
	auto res = inet_pton(AF_INET, srv_addr, &ip);
	auto SA = address(*sock);
	ASSERT_EQ(ip.s_addr, SA->sin_addr.s_addr);
	ASSERT_EQ(ip.s_addr, SA->sin_addr.s_addr);
	auto res_sockaddr = reinterpret_cast<struct sockaddr_in*>(&addr);
	ASSERT_EQ(res_sockaddr->sin_port, SA->sin_port);

	// convert addresses back to text form and check string equality
  char res_nwbuf[INET_ADDRSTRLEN];
  char nwbuf[INET_ADDRSTRLEN];
	auto res_nwaddr = inet_ntop(AF_INET, &(res_sockaddr->sin_addr), res_nwbuf, INET_ADDRSTRLEN);
	auto nw_addr = inet_ntop(AF_INET, &(SA->sin_addr), nwbuf, INET_ADDRSTRLEN);
	ASSERT_STREQ(res_nwbuf, nwbuf);
}


TEST(socket_utils, GetSocketFamilyFromSocketDescriptor) {
	auto sock_uptr { socket_tcp::
		create()->socket()->bind(SERVICE)->build() };
	auto family = sockfd_to_family(sockno(*sock_uptr));
	ASSERT_EQ(family, AF_INET);
}

TEST(socket_state, CanStoreErrorFlags) {
	auto err = SockError::ERR_SOCKET | SockError::ERR_CONNECT;
	ASSERT_FALSE(toUType(err & SockError::ERR_BIND) != 0);
	err |= SockError::ERR_BIND;
	ASSERT_TRUE(toUType(err & SockError::ERR_BIND) != 0);
}

#ifdef SYS_API_LINUX
using socket_local = wasl_socket<AF_UNIX, SOCK_DGRAM>;

TEST(socket_builder_unix_domain, CanBuildUnixDomainDatagram) {
	std::unique_ptr<socket_local> sock { socket_local::
		create()->socket()->bind(srv_path)->build() };

	ASSERT_TRUE(is_open(*sock));
	ASSERT_NE(sockno(*sock), INVALID_SOCKET);
	ASSERT_TRUE(sock);
}

TEST(socket_builder_unix_domain, MakeHelperUnixDomainDatagram) {
	auto sock { make_socket<AF_LOCAL, SOCK_DGRAM>(srv_path) };
	ASSERT_NE(sockno(*sock), INVALID_SOCKET);
  ASSERT_TRUE(is_open(*sock));
}

TEST(unix_domain_sockets, CanReadAndWrite) {
	auto msg = "some_message"s;
//	auto srvUP { make_socket<sockaddr_un, SOCK_DGRAM>(srv_path) };
//	auto srv_fd { sockno(*srvUP) };


#if 0
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
#endif
}
#endif


#if 0
TEST(socket, CanConnectToPeerOnSameHost) {
	auto s1 { make_socket<AF_LOCAL, SOCK_DGRAM>(srv_path) };
	auto s2 { make_socket<AF_LOCAL, SOCK_DGRAM>(client_path) };
}


#endif




TEST(socket_builder_tcp, CanBuildTCPSocketWithPortOnly) {
	std::unique_ptr<socket_tcp> sockUP { socket_tcp::
    create()->socket()->bind(SERVICE)->build() };

	socket_listen(*sockUP);

	ASSERT_NE(sockno(*sockUP), INVALID_SOCKET);
	ASSERT_TRUE(is_open(*sockUP));
}

TEST(socket_builder_tcp, CanBuildTCPSocketWithPortAndIPAddress) {
	std::unique_ptr<socket_tcp> sockUP { socket_tcp::
    create()->socket()->bind(SERVICE, srv_addr)->build() };

	socket_listen(*sockUP);

	ASSERT_TRUE(is_open(*sockUP));
}

TEST(socket_tcp, CanReceiveDataFromClient) {
	auto msg = "abc\n123."s;
	// create listening socket
	auto srvUP { make_socket<AF_INET, SOCK_STREAM>(SERVICE, srv_addr) };

	// launch client
	std::stringstream cmd;
	cmd << "perl ./test/scripts/tcp_client.pl " << HOST << " " << SERVICE << " '" << msg << "'";
	wasl::run_process<wasl::platform_type>(cmd.str().c_str());

	// accept client
	auto client_fd = socket_accept(srvUP.get());

	// read data
	char buf[256];
	recvfrom(client_fd ,buf, 256, 0, NULL, NULL);
	ASSERT_TRUE(strstr(buf, msg.c_str()));
}


TEST(socket_builder_udp, CanBuildUDPSocketWithPortOnly) {
	std::unique_ptr<socket_udp> sockUP { socket_udp::
    create()->socket()->bind(SERVICE)->build() };

	socket_listen(*sockUP);

	ASSERT_NE(sockno(*sockUP), INVALID_SOCKET);
	ASSERT_TRUE(is_open(*sockUP));
}

TEST(socket_builder_udp, CanBuildUDPSocketWithPortAndIPAddress) {
	std::unique_ptr<socket_udp> sockUP { socket_udp::
    create()->socket()->bind(SERVICE, srv_addr)->build() };

	socket_listen(*sockUP);

	ASSERT_TRUE(is_open(*sockUP));
}


TEST(socket_udp, CanReceiveDataFromClient) {
	auto msg = "bye"s;
	// create listening socket
	auto srvUP { make_socket<AF_INET, SOCK_DGRAM>(SERVICE, srv_addr) };

	// launch client
	std::stringstream cmd;
	cmd << "perl ./test/scripts/udp_client.pl " << HOST << " " << SERVICE;

	auto cpid = fork();
	ASSERT_NE(cpid, -1);

	if (cpid == 0) {
		// run in child process
		wasl::run_process<wasl::platform_type>(cmd.str().c_str());
	}

//	wasl::run_process<wasl::platform_type>(cmd.str().c_str());
	// read data
	char buf[256];
	struct sockaddr addr;
	socklen_t len = sizeof(sockaddr_in);
	recvfrom(sockno(*srvUP), buf, 256, 0, &addr, &len);
	std::cout << "buf: " << buf << '\n';
	ASSERT_TRUE(strstr(buf, msg.c_str()));
}





