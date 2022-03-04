#include <gsl/span>
#include "test_helpers.h"

/*
 * Launch another process as a daemon
 * ./daemonize path_to_executable [args...]
 */

int main(int argc, char **argv)
{
	// below should only verify server is running otherwise the net syscalls
	// will block. Run the process as a separate command.
	std::vector<gsl::czstring<>> args (argc - 2);
	for (size_t i {2}; i < argc; ++i) {
		args[i - 2] = argv[i];
	}

	auto res = wasl::run_process<wasl::platform_type>(argv[1], args, false);
	return res;
}
