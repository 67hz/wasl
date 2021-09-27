#include <wasl/SockStream.h>
#include <wasl/Socket.h>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace wasl::ip;

TEST(sockstream, CanDeferSocketLoading) {
	auto sockUP { make_socket<sockaddr_un, SOCK_DGRAM>("/tmp/wasl/srv") };
	ASSERT_TRUE(std::is_default_constructible<sockstream>::value);
	sockstream lazy_ss;
	lazy_ss.set_handle(sockno(*sockUP));
	ASSERT_EQ(sockno(lazy_ss), sockno(*sockUP));
}

TEST(sockstream, IsMoveConstructible) {
	auto sockUP { make_socket<sockaddr_un, SOCK_DGRAM>("/tmp/wasl/srv") };
	std::unique_ptr<sockstream>ss1 = sdopen(sockno(*sockUP));
	auto ss2 {std::move(ss1)};
	ASSERT_TRUE(ss2);
	ASSERT_NE(sockno(*ss2), INVALID_SOCKET);
}

TEST(sockstream, IsMoveAssignable) {
	auto sock1 { make_socket<sockaddr_un, SOCK_DGRAM>("/tmp/wasl/srv") };
	auto sock2 { make_socket<sockaddr_un, SOCK_DGRAM>("/tmp/wasl/cl") };
  sockstream ss1(sockno(*sock1));
	auto ss1_fd = sockno(ss1);

  sockstream ss2(sockno(*sock2));
	ss2 = std::move(ss1);

	ASSERT_TRUE(ss2);
	ASSERT_EQ(sockno(ss2), ss1_fd);
}
