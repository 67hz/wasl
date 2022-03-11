#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include <gsl/span>

#include "test_helpers.h"
#include "wasl/Types.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace wasl::ip;

using socket_tcp = wasl_socket<AF_INET, SOCK_STREAM>;



// TODO make these a fixure that to remove redundant client build
TEST(socket_client_tcp, GetAddressFromSocketDescriptor) {
	auto client {
		std::make_unique<wasl_socket<AF_INET, SOCK_STREAM>>() };
	socket_connect(*client, {HOST, SERVICE});

	auto addr = get_address(sockno(*client));
	in_addr ip;
	auto res = inet_pton(AF_INET, HOST, &ip);
	auto SA = as_address(*client);
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

TEST(socket_client_tcp, CanReceiveAndSendDataFromServer) {
	auto client {
		std::make_unique<wasl_socket<AF_INET, SOCK_STREAM>>() };
	socket_connect(*client, {HOST, SERVICE});

	char buf[BUFSIZ];
	struct sockaddr_in addr;
	auto ss_cl { sdopen(sockno(*client)) };

	gsl::czstring<> msg {"sendingdata"};
	*ss_cl << msg << std::endl;

	*ss_cl >> buf;
	ASSERT_STREQ(buf, msg);
}

