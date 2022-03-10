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
  auto client { make_socket<AF_UNIX, SOCK_DGRAM>({tmpSock.c_str()}) };
  client->connect(addr_info);

	EXPECT_EQ(client->error(), SockError::ERR_NONE);

	auto ss { sdopen(sockno(*client)) };
	*ss << "hello" << std::endl;
}
