#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include "test_helpers.h"
using namespace wasl::ip;

int main(int argc, char *argv[])
{
	// below should only verify server is running otherwise the net syscalls
	// will block. Run the process as a separate command.
	std::vector<gsl::czstring<>> args = {argv[1], argv[2]};
	auto res = wasl::run_process<wasl::platform_type>("./echo_server_stream", args, false);
	std::cout << "returning from process driver: " << res << '\n';
	return 0;
}
