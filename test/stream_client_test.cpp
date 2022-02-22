#include <wasl/Socket.h>
#include <wasl/SockStream.h>
#include "test_helpers.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace wasl::ip;

TEST(socket_client_tcp, CanReceiveAndSendDataFromServer) {

		auto client {
			wasl_socket<AF_INET, SOCK_STREAM>::create()
				->socket ()
				->connect ({SERVICE, HOST})
				->build()
		};

		struct sockaddr_in addr;
		//auto server = getpeername(sockno(*client), (struct sockaddr*)&addr, sizeof(addr));
		auto ss_cl { sdopen(sockno(*client)) };

		*ss_cl << "sending_data_to_server" << std::endl;

}


