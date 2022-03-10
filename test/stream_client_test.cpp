#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include <gsl/span>

#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace wasl::ip;

using socket_tcp = wasl_socket<AF_INET, SOCK_STREAM>;



// TODO make these a fixure that to remove redundant client build
TEST(socket_client_tcp, GetAddressFromSocketDescriptor) {
	auto client {
		wasl_socket<AF_INET, SOCK_STREAM>::create()
			->connect ({HOST, SERVICE})
			->build()
	};
#if 0
	auto addr = get_address(sockno(*sock));
	in_addr ip;
	auto res = inet_pton(AF_INET, HOST, &ip);
	auto SA = as_address(*sock);
	ASSERT_EQ(ip.s_addr, SA->sin_addr.s_addr);
	auto res_sockaddr = reinterpret_cast<struct sockaddr_in*>(&addr);
	ASSERT_EQ(res_sockaddr->sin_port, SA->sin_port);

	// convert addresses back to text form and check string equality
	char res_nwbuf[INET_ADDRSTRLEN];
	char nwbuf[INET_ADDRSTRLEN];
	auto res_nwaddr = inet_ntop(AF_INET, &(res_sockaddr->sin_addr), res_nwbuf, INET_ADDRSTRLEN);
	auto nw_addr = inet_ntop(AF_INET, &(SA->sin_addr), nwbuf, INET_ADDRSTRLEN);
	ASSERT_STREQ(res_nwbuf, nwbuf);
#endif
}

TEST(socket_client_tcp, CanReceiveAndSendDataFromServer) {
	auto client {
		wasl_socket<AF_INET, SOCK_STREAM>::create()
			->connect ({HOST, SERVICE})
			->build()
	};

	EXPECT_EQ(client->error(), SockError::ERR_NONE);

	char buf[BUFSIZ];
	struct sockaddr_in addr;
	auto ss_cl { sdopen(sockno(*client)) };

	gsl::czstring<> msg {"sendingdata"};
	*ss_cl << msg << std::endl;

	*ss_cl >> buf;
	ASSERT_STREQ(buf, msg);
}
