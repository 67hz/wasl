#include <wasl/SockStream.h>
#include <wasl/Socket.h>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace wasl::ip;




TEST(sockstream, CanDeferSocketLoading) {
	auto sockUP { make_socket<AF_INET6, SOCK_STREAM>({.service = SERVICE, .host = HOST6}) };
	ASSERT_TRUE(std::is_default_constructible<sockstream>::value);
	sockstream lazy_ss;
	lazy_ss.set_handle(sockno(*sockUP));
	ASSERT_EQ(sockno(lazy_ss), sockno(*sockUP));
}

TEST(sockstream, IsMoveConstructible) {
	auto sockUP { make_socket<AF_UNIX, SOCK_STREAM>({.host = SRV_PATH}) };
	std::unique_ptr<sockstream>ss1 = sdopen(sockno(*sockUP));
	auto ss2 {std::move(ss1)};
	ASSERT_TRUE(ss2);
	ASSERT_NE(sockno(*ss2), INVALID_SOCKET);
}

#if 0
TEST(sockstream, IsMoveAssignable) {
	auto sock1 { make_socket<AF_INET, SOCK_DGRAM>({.host = SRV_PATH}) };
	auto sock2 { make_socket<AF_INET, SOCK_DGRAM>({.host = CL_PATH}) };
  sockstream ss1(sockno(*sock1));
	auto ss1_fd = sockno(ss1);

  sockstream ss2(sockno(*sock2));
	ss2 = std::move(ss1);

	ASSERT_TRUE(ss2);
	ASSERT_EQ(sockno(ss2), ss1_fd);
}

TEST(sockstream_tcp_stream, CanReadData) {
	auto srv { make_socket<AF_INET, SOCK_STREAM>(SERVICE,HOST) };

	auto msg_from_client {"abc1234"};

	std::stringstream cmd;
	cmd << "perl ./test/scripts/tcp_client.pl " << HOST << " " << SERVICE << " '" << msg_from_client << "'";
	wasl::run_process<wasl::platform_type>(cmd.str().c_str());

	auto client_fd = socket_accept(srv.get());
	ASSERT_NE(client_fd, -1);
//	sockstream ss_cl {client_fd}; // same as below
	auto ss_cl = sdopen(client_fd);

	std::string res;
	*ss_cl >> res;
	ASSERT_STREQ(res.c_str(), msg_from_client);
}

#if 0
TEST(sockstream_tcp_stream, CanSendClientData) {
	auto srv { make_socket<AF_INET, SOCK_STREAM>(SERVICE,HOST) };
	sockstream ss_srv {sockno(*srv)};
	ASSERT_EQ(sockno(ss_srv), sockno(*srv));

	auto msg {"abc1234"};
	std::stringstream cmd;
	cmd << "perl ./test/scripts/tcp_client_receiving.pl " << HOST << " " << SERVICE << " '" << msg << "'";
	wasl::run_process<wasl::platform_type>(cmd.str().c_str());

	auto client_fd = socket_accept(srv.get());
	ASSERT_NE(client_fd, -1);
	sockstream ss_cl {client_fd};

	std::string res;

	ss_cl >> res;
	ASSERT_STREQ(res.c_str(), msg);

	// write to client
//	ss_cl << "can you see me?" << std::endl;
}
#endif
#endif
