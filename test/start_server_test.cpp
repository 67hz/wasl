#include <wasl/Socket.h>
#include <wasl/SockStream.h>

#include <gsl/span>
#include "test_helpers.h"
using namespace wasl::ip;

int main(int argc, char **argv)
{
	// below should only verify server is running otherwise the net syscalls
	// will block. Run the process as a separate command.
	std::vector<gsl::czstring<>> args (argc - 1);
	for (size_t i {1}; i < argc; ++i) {
		args[i - 1] = argv[i];
	}

//	gsl::span<gsl::zstring<>> args = gsl::make_span(argv, argc);
	auto res = wasl::run_process<wasl::platform_type>("./echo_server_stream", args, false);
//	std::cout << "returning from process driver: " << res << '\n';
	return 0;
}
