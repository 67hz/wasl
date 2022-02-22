#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include "test_helpers.h"
#include <gtest/gtest.h>
using namespace testing;


using namespace wasl::ip;

TEST(socket_tcp_server, CanStartServer) {
	auto server { make_socket<AF_INET, SOCK_STREAM>({
							.service = SERVICE,
							.host = HOST,
							.reuse_addr = true}) };
	socket_listen(*server);

	ASSERT_TRUE(is_valid_socket(sockno(*server)));

	char buf[BUFSIZ];

	auto client_fd = socket_accept(server.get());
	while (strstr(buf, "exit") == NULL) {

		// open a IO socketstream
		auto client_ss = sdopen(client_fd);

		// read from clients
		*client_ss >> buf;

		// echo back
		*client_ss << buf << std::endl;
	}

}
