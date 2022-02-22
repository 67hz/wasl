#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include "test_helpers.h"
#include <gtest/gtest.h>
using namespace testing;


using namespace wasl::ip;

TEST(socket_tcp_server, CanStartEchoServer) {
	std::vector<gsl::czstring<>> args = {HOST, SERVICE};
	wasl::run_process<wasl::platform_type>("./test/echo_server_stream", args, false);
}
