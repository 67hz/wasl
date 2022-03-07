#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include <gsl/span>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace wasl::ip;

TEST(socket_client_unix_datagram, CanReceiveAndSendDataFromServer) {
	Address_info addr_info;
	addr_info.host = SRV_PATH;
	auto tmpSock = wasl::makeSocketPath();
	auto client {
		wasl_socket<AF_UNIX, SOCK_DGRAM>::create()
			->bind({.host = tmpSock})
			->connect (addr_info)
			->build()
	};

	EXPECT_EQ(client->sock_err, SockError::ERR_NONE);

	auto ss { sdopen(sockno(*client)) };
	*ss << "hello" << std::endl;
}
