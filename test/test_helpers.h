#ifndef WASL_TEST_HELPERS_H
#define WASL_TEST_HELPERS_H

#include <thread>

#include <gtest/gtest.h>

#include <wasl/Common.h>

#if defined(SYS_API_WIN32)
#include <Windows.h>

#elif defined(SYS_API_LINUX)
#include <sys/wait.h>
#include <unistd.h>

#define SRV_PATH "/tmp/wasl/srv"
#define CL_PATH "/tmp/wasl/cl"
#define HOST "127.0.0.1"
#define SERVICE "9877"

static int wait_for_child_fork(int pid) {
  int status;
  if (waitpid(pid, &status, 0) < 0)
    return -1;
  if (WIFEXITED(status)) // normal exit
    return WEXITSTATUS(status);
  else
    return -2;
}

template <typename F1, typename F2> auto fork_and_wait(F1&& f1, F2&& f2) {
  pid_t cpid;
  cpid = fork();
  if (cpid == -1) {
    FAIL() << "fork";
  }

  if (cpid == 0) { // child context
    f2();
    exit(testing::Test::HasFailure());
  } else { // parent context
    f1();
    ASSERT_EQ(0, wait_for_child_fork(cpid));
    exit(testing::Test::HasFailure());
  }
}

// TODO LamStop73 noexcept(noexcept(...))
template <typename F, typename... Types,
          typename = std::enable_if_t<(sizeof...(Types) > 1)>>
auto fork_and_wait(F f, Types&&... args) {

  auto fw_impl = [&]() {
    pid_t cpid;
    cpid = fork();
    if (cpid == -1) {
      FAIL() << "fork";
    }

    if (cpid == 0) { // child context
      fork_and_wait(std::forward<Types>(args)...);
    } else { // parent context
      f();
      ASSERT_EQ(0, wait_for_child_fork(cpid));
      exit(testing::Test::HasFailure());
    }
  };

  fw_impl();
}

class thread_guard {
  std::thread &t;

public:
  explicit thread_guard(std::thread &t_) : t{t_} {}
  ~thread_guard() {
    if (t.joinable())
      t.join();

    exit(testing::Test::HasFailure());
  }
  thread_guard(thread_guard const &) = delete;
  thread_guard &operator=(thread_guard const &) = delete;
};

#endif // fork-based posix helpers



namespace wasl {

template <typename P,
				 EnableIfSamePlatform<P, posix> = true>
constexpr int run_process(const char* command) {
	int ret = system(command);
	/*
	if (WIFSIGNALED(ret) &&
			(WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
			break;
			*/
	return 0;
}

#ifdef SYS_API_WIN32
// Windows uses CreateProcess in lieue of fork()
template <typename P,
				 EnableIfSamePlatform<P, windows> = true>
constexpr int run_process(const char* command) {
  STARTUPINFO si { sizeof(si) };
  PROCESS_INFORMATION pi;
  auto cmd = const_cast<char*>(command);

  bool success = ::CreateProcess(nullptr, TEXT(cmd), nullptr, nullptr, FALSE, 0,
      nullptr, nullptr, &si, &pi);
  if (success) {
    // close thread handle
    ::CloseHandle(pi.hThread);
    ::WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process handle
    ::CloseHandle(pi.hProcess);
  }

	return 0;
}
#endif



} // namespace wasl

#endif /* ifndef WASL_TEST_HELPERS_H */
