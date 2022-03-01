#include <wasl/SockStream.h>
#include <wasl/Socket.h>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace wasl::ip;


TEST(sockstream, CanDeferSocketLoading) {
	auto sockUP { make_socket<AF_INET6, SOCK_STREAM>({HOST6, SERVICE}) };
	ASSERT_TRUE(std::is_default_constructible<sockstream>::value);
	sockstream lazy_ss;
	lazy_ss.set_handle(sockno(*sockUP));
	ASSERT_EQ(sockno(lazy_ss), sockno(*sockUP));
}

TEST(sockstream, IsMoveConstructible) {
	auto sockUP { make_socket<AF_UNIX, SOCK_STREAM>({SRV_PATH}) };
	std::unique_ptr<sockstream>ss1 = sdopen(sockno(*sockUP));
	auto ss2 {std::move(ss1)};
	ASSERT_TRUE(ss2);
	ASSERT_NE(sockno(*ss2), INVALID_SOCKET);
}

TEST(sockstream, IsMoveAssignable) {
	auto sock1 { make_socket<AF_INET, SOCK_DGRAM>({HOST}) };
	auto sock2 { make_socket<AF_INET, SOCK_DGRAM>({HOST}) };
  sockstream ss1(sockno(*sock1));
	auto ss1_fd = sockno(ss1);

  sockstream ss2(sockno(*sock2));
	ss2 = std::move(ss1);

	ASSERT_TRUE(ss2);
	ASSERT_EQ(sockno(ss2), ss1_fd);
}

#if 0

TEST(sockstream_datagram, CanReadClientData) {
	auto srv { make_socket<AF_INET, SOCK_DGRAM>({SERVICE,HOST}) };

	sockstream ss_srv {sockno(*srv)};
	ASSERT_EQ(sockno(ss_srv), sockno(*srv));

	const char msg[] = "abc1234";
  std::vector<gsl::czstring<>> args = {"./test/scripts/udp_client.pl", HOST, SERVICE, msg};
	wasl::run_process<wasl::platform_type>("perl", args, false);

	std::string res;

	ss_srv >> res;
	ASSERT_STREQ(res.c_str(), msg);

	// write to client
//	ss_cl << "can you see me?" << std::endl;
}
#endif
