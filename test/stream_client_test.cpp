#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include <gsl/span>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace wasl::ip;

TEST(socket_client_tcp, CanReceiveAndSendDataFromServer) {
	auto client {
		wasl_socket<AF_INET, SOCK_STREAM>::create()
			->connect ({HOST, SERVICE})
			->build()
	};

	EXPECT_EQ(client->sock_err, SockError::ERR_NONE);

	char buf[BUFSIZ];
	struct sockaddr_in addr;
	auto ss_cl { sdopen(sockno(*client)) };

	gsl::czstring<> msg {"sendingdata"};
	*ss_cl << msg << std::endl;

	*ss_cl >> buf;
	ASSERT_STREQ(buf, msg);

}
