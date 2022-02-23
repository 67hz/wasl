#ifndef WASL_TEST_HELPERS_H
#include <thread>
#include <gsl/string_span> // czstring

#include <wasl/Common.h>

#define SRV_PATH "/tmp/wasl_srv"
#define CL_PATH "/tmp/wasl_cl"
#define HOST "127.0.0.1"
#define HOST6 "::1"
#define SERVICE "9877"

#if defined(SYS_API_WIN32)
#include <Windows.h>
#include <strsafe.h>

#elif defined(SYS_API_LINUX)
#include <sys/wait.h>
#include <unistd.h>
#endif



namespace wasl {

class thread_guard {
  std::thread &t;

public:
  explicit thread_guard(std::thread &t_) : t{t_} {}
  ~thread_guard() {
    if (t.joinable())
      t.join();
  }
  thread_guard(thread_guard const &) = delete;
  thread_guard &operator=(thread_guard const &) = delete;
};

#ifdef SYS_API_LINUX

static int wait_for_pid_fork(int pid) {
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
		return -1;
  }

  if (cpid == 0) { // child context
    f2();
    exit(EXIT_SUCCESS);
  } else { // parent context (original callee context)
    f1();
		if (wait_for_pid_fork(cpid) == 0)
			exit(EXIT_SUCCESS);
		else
			exit(EXIT_FAILURE);
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
			return -1;
    }

    if (cpid == 0) { // grand child context
      fork_and_wait(std::forward<Types>(args)...);
    } else { // child context
      f();
			if (wait_for_pid_fork(cpid) == 0)
				exit(EXIT_SUCCESS);
			else
				exit(EXIT_FAILURE);
    }
  };

  fw_impl();
}

template <typename P,
				 EnableIfSamePlatform<P, posix> = true>
int run_process(const char* command, std::vector<gsl::czstring<>> args, bool wait_child = false) {
	/*
	if (WIFSIGNALED(ret) &&
			(WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
			break;
			*/

	char* arg_arr[args.size() + 1]; // allow extra index for tail nullptr
	// execvp* requires nullptr terminated
	arg_arr[0] = const_cast<char*>(command); // posix takes command as first args arg by convention
	arg_arr[args.size()+1] = nullptr;

	for (auto i {1}; i < args.size()+1; ++i) {
		arg_arr[i] = const_cast<char*>(args[i-1]);
	}

	pid_t cpid = fork();
	if (cpid == -1) {
		return -1;
	}

	if (cpid == 0) { // child context
		int ret = execvp(command, arg_arr);

		if (ret == -1)
			return -1;
		else
			return 0;
	}
	else { // parent context
		if (wait_child) {
			if (wait_for_pid_fork(cpid) == 0)
				exit(EXIT_SUCCESS);
			else
				exit(EXIT_FAILURE);
		} else {
			exit(EXIT_SUCCESS);
		}
	}
}

#endif // SYS_API_LINUX

#ifdef SYS_API_WIN32

void pr_err(LPTSTR lpszFunction) {
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError();
  DWORD system_locale = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

  FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr,
      dw,
      system_locale,
      (LPTSTR) &lpMsgBuf,
      0, nullptr);

  lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
      (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) + sizeof(TCHAR));
  StringCchPrintf((LPTSTR)lpDisplayBuf,
      LocalSize(lpDisplayBuf) / sizeof(TCHAR),
      TEXT("%s failed with error: %d: %s"),
      lpszFunction, dw, lpMsgBuf);

  std::cout << (LPCTSTR)lpDisplayBuf << '\n';
}


// Windows uses CreateProcess in lieue of fork/exec
template <typename P,
				 EnableIfSamePlatform<P, windows> = true>
auto run_process(const char* command, std::vector<gsl::czstring<>> args, bool wait_child = false) {
  STARTUPINFO si { sizeof(si) };
  PROCESS_INFORMATION pi;

  // windows takes command line string as executable lus args
  args.insert(args.begin(), command);

  std::ostringstream cmd;
  std::copy(args.begin(), args.end(), std::ostream_iterator<gsl::czstring<>>(cmd, " "));

  bool success = ::CreateProcess(nullptr, TEXT(const_cast<char*>(cmd.str().c_str())), nullptr, nullptr, FALSE, 0,
      nullptr, nullptr, &si, &pi);
  if (success) {
    // close thread handle
    ::CloseHandle(pi.hThread);
		if (wait_child) {
			::WaitForSingleObject(pi.hProcess, INFINITE);
    }

    // Close process handle
    ::CloseHandle(pi.hProcess);
  } else {
    pr_err(TEXT("Wasl Test Helper: Run Process"));
  }
}
#endif



} // namespace wasl

#endif /* ifndef WASL_TEST_HELPERS_H */
