#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include "test_helpers.h"
#include <gtest/gtest.h>
using namespace wasl::ip;

TEST(start_server_tcp, CanStartEchoServer) {
	std::vector<gsl::czstring<>> args = {HOST, SERVICE};
	wasl::run_process<wasl::platform_type>("./echo_server_stream", args, false);
	std::cout << "returning from process driver\n";
}
