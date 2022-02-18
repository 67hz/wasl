#include <wasl/Socket.h>
#include <string>
#include <memory>

#ifdef SYS_API_LINUX
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif

#include <type_traits>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace testing;

using namespace std::string_literals;
using namespace wasl::ip;
using wasl::local::toUType;

constexpr gsl::czstring<> srv_addr{HOST};

using socket_tcp = wasl_socket<AF_INET, SOCK_STREAM>;
using socket_udp = wasl_socket<AF_INET, SOCK_DGRAM>;
using socket_dgram = wasl_socket<AF_INET, SOCK_DGRAM>;

TEST(socket_utils, GetAddressFromSocketDescriptor) {
	std::unique_ptr<socket_tcp> sock { socket_tcp::
		create()->socket()->bind({SERVICE, srv_addr})->build() };
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


TEST(socket_utils, GetStreamSocketFamilyFromSocketDescriptor) {
	auto sock_tcp { socket_tcp::
		create()->socket()->bind({SERVICE})->build() };
	auto family = sockfd_to_family(sockno(*sock_tcp));
	ASSERT_EQ(family, AF_INET);
}

TEST(socket_state, CanStoreErrorFlags) {
	auto err = SockError::ERR_SOCKET | SockError::ERR_CONNECT;
	ASSERT_FALSE(toUType(err & SockError::ERR_BIND) != 0);
	err |= SockError::ERR_BIND;
	ASSERT_TRUE(toUType(err & SockError::ERR_BIND) != 0);
}

#ifdef SYS_API_LINUX
using socket_local_dgram = wasl_socket<AF_UNIX, SOCK_DGRAM>;
using socket_local_stream = wasl_socket<AF_UNIX, SOCK_STREAM>;
constexpr gsl::czstring<> srv_path{SRV_PATH};
constexpr gsl::czstring<> client_path{CL_PATH};


static void assert_sock_path_exists(gsl::czstring<> sock_path) {
	// check existence of path in system
	struct stat sb;
	auto file_info = lstat(srv_path, &sb);
	ASSERT_NE(file_info, -1);
	ASSERT_EQ((sb.st_mode & S_IFMT), S_IFSOCK);
}

TEST(socket_utils, GetUnixSocketFamilyFromSocketDescriptor) {
	auto sock_unix { socket_local_stream::
		create()->socket()->bind({.host = srv_path})->build() };
	auto family = sockfd_to_family(sockno(*sock_unix));
	ASSERT_EQ(family, AF_UNIX);
}

TEST(socket_builder_unix_domain, CanBuildUnixDomainDatagram) {
	std::unique_ptr<socket_local_dgram> sock { socket_local_dgram::
		create()->socket()->bind({.host = srv_path})->build() };

	ASSERT_TRUE(is_open(*sock));
	ASSERT_NE(sockno(*sock), INVALID_SOCKET);
	ASSERT_TRUE(sock);

	assert_sock_path_exists(srv_path);

}

TEST(socket_builder_unix_domain, CanBuildUnixDomainStream) {
	std::unique_ptr<socket_local_stream> sock { socket_local_stream::
		create()->socket()->bind({.host = srv_path})->build() };

	ASSERT_TRUE(is_open(*sock));
	ASSERT_NE(sockno(*sock), INVALID_SOCKET);

	assert_sock_path_exists(srv_path);
}

TEST(socket_builder_unix_domain, MakeHelperUnixDomainDatagram) {
	auto sock { make_socket<AF_LOCAL, SOCK_DGRAM>({.host = srv_path}) };
	ASSERT_NE(sockno(*sock), INVALID_SOCKET);
  ASSERT_TRUE(is_open(*sock));
	assert_sock_path_exists(srv_path);
}

TEST(socket_builder_unix_domain, MakeHelperUnixDomainStream) {
	auto sock { make_socket<AF_LOCAL, SOCK_STREAM>({.host = srv_path}) };
	ASSERT_NE(sockno(*sock), INVALID_SOCKET);
  ASSERT_TRUE(is_open(*sock));
	assert_sock_path_exists(srv_path);
}

TEST(unix_domain_sockets_datagram, CanReadAndWrite) {
	char msg[]= "some_message";
	auto srvUP { make_socket<AF_LOCAL, SOCK_DGRAM>({.host = srv_path}) };
	auto srv_fd { sockno(*srvUP) };

	wasl::fork_and_wait([srv = *srvUP, msg] {
		// parent receives
		char buf[BUFSIZ];
		struct sockaddr_un claddr;
		socklen_t len;

		auto num_bytes = recvfrom(sockno(srv), buf, BUFSIZ, 0,
				(struct sockaddr *) &claddr, &len);
		ASSERT_STREQ(buf, msg);

	}, [msg, srv = *srvUP]() {
		// child sends
		auto client_sock { make_socket<AF_LOCAL, SOCK_DGRAM>({.host= client_path}) };
		std::unique_ptr<struct sockaddr_un> srv_addr {address(srv)};
		if (sendto(sockno(*client_sock), msg, sizeof(msg), 0, (struct sockaddr *)(srv_addr.get()), sizeof(struct sockaddr_un)) == -1)
			std::cerr << strerror(errno) << '\n';
	});
}

TEST(unix_domain_sockets_stream, CanReadAndWrite) {
	char msg[]= "some_message";
	auto srvUP { make_socket<AF_LOCAL, SOCK_STREAM>({.host = srv_path}) };
	auto srv_fd { sockno(*srvUP) };

	wasl::fork_and_wait([srv = *srvUP, msg] {
		// parent receives
		char buf[BUFSIZ];
		struct sockaddr_un claddr;
		socklen_t len;

		socket_listen(srv);
		socket_accept(&srv);

		auto num_bytes = recvfrom(sockno(srv), buf, BUFSIZ, 0,
				(struct sockaddr *) &claddr, &len);
		ASSERT_STREQ(buf, msg);

	}, [msg, srv = *srvUP]() {
		// child sends
		auto client_sock { make_socket<AF_LOCAL, SOCK_STREAM>({.host = client_path}) };


		if (socket_connect(*client_sock, sockno(srv)) == -1)
			FAIL() << strerror(errno);

		if (write(sockno(*client_sock), msg, sizeof(msg)) != sizeof(msg))
			std::cerr << "write error: " << strerror(errno) << '\n';
	});
}
#endif



TEST(socket_builder_tcp, CanBuildTCPSocketWithPortOnly) {
	//std::unique_ptr<socket_tcp> sockUP { socket_tcp::
  //  create()->socket()->bind(SERVICE)->build() };
	auto sockUP = make_socket<AF_INET, SOCK_STREAM>({.service = SERVICE});

	socket_listen(*sockUP);

	ASSERT_NE(sockno(*sockUP), INVALID_SOCKET);
	ASSERT_TRUE(is_open(*sockUP));
}

TEST(socket_builder_tcp, CanBuildTCPSocketWithPortAndIPAddress) {
	std::unique_ptr<socket_tcp> sockUP { socket_tcp::
    create()->socket()->bind({SERVICE, srv_addr})->build() };

	socket_listen(*sockUP);

	ASSERT_TRUE(is_open(*sockUP));
}

TEST(socket_tcp, CanReceiveDataFromClient) {
	const char msg[] = "abc123\n";
	// create listening socket
	auto srvUP { make_socket<AF_INET6, SOCK_STREAM>({.service = SERVICE, .host = HOST6}) };
	socket_listen(*srvUP);

	// launch client
	std::stringstream cmd;
  std::vector<gsl::czstring<>> args = {"./test/scripts/tcp_client.pl", HOST6, SERVICE, msg};
	wasl::run_process<wasl::platform_type>("perl", args, false);

	// accept client
	auto client_fd = socket_accept(srvUP.get());

	// read data
	char buf[256];
	recvfrom(client_fd ,buf, 256, 0, nullptr, nullptr);
	std::cout << "buf: " << buf << "\n";
	ASSERT_TRUE(strstr(buf, msg));
}

TEST(socket_builder_udp, CanBuildUDPSocketWithPortOnly) {
	std::unique_ptr<socket_udp> sockUP { socket_udp::
    create()->socket()->bind({SERVICE})->build() };

	socket_listen(*sockUP);
	ASSERT_NE(sockno(*sockUP), INVALID_SOCKET);
	ASSERT_TRUE(is_open(*sockUP));
}

TEST(socket_builder_udp, CanBuildUDPSocketWithPortAndIPAddress) {
	std::unique_ptr<socket_udp> sockUP { socket_udp::
    create()->socket()->bind({.service =  SERVICE, .host = srv_addr})->build() };

	socket_listen(*sockUP);

	ASSERT_TRUE(is_open(*sockUP));
}

TEST(socket_udp, CanReceiveAndSendDataFromClient) {
	char send_buf[] = "test message\n";
	const char terminate_msg[] = "bye";

	auto server { make_socket<AF_INET, SOCK_DGRAM>({SERVICE, srv_addr}) };

  std::vector<gsl::czstring<>> args = {"./test/scripts/udp_client.pl", HOST, SERVICE, terminate_msg};
	wasl::run_process<wasl::platform_type>("perl", args, false);

	char recv_buf[256];
	struct sockaddr addr;
	socklen_t len = sizeof(sockaddr_in);

	recvfrom(sockno(*server), recv_buf, sizeof(recv_buf), 0, &addr, &len);
	ASSERT_STREQ(recv_buf, "howdy\n");

	sendto(sockno(*server), send_buf, sizeof(send_buf), 0, &addr, len);
	recvfrom(sockno(*server), recv_buf, sizeof(recv_buf), 0, &addr, &len);
	ASSERT_TRUE(strstr(recv_buf, send_buf));

	sendto(sockno(*server), terminate_msg, sizeof(terminate_msg), 0, &addr, len);
}
