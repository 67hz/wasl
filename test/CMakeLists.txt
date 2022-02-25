include(GNUInstallDirs)
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.11.0
)

FetchContent_GetProperties(googletest)

if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)

  # leave this on for Posix - weird GMock issues surface without pthreads
  set(gtest_disable_pthreads OFF CACHE BOOL "" FORCE)

  if (WIN32)
    set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  elseif(UNIX)
    find_package(Threads REQUIRED)
  endif()

  add_subdirectory(
    ${googletest_SOURCE_DIR}
    ${googletest_BINARY_DIR}
    )
  FetchContent_MakeAvailable(googletest)
endif()

cmake_print_variables(googletest_BINARY_DIR)
cmake_print_variables(googletest_SOURCE_DIR)

include(GoogleTest)
#include(CTest)
#enable_testing()

set(TEST_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
file(TO_NATIVE_PATH ${TEST_SCRIPTS_DIR} testScripts)
cmake_print_variables(test_helpers)


add_executable(echo_server_stream nodes/echo_server_stream.cpp)
target_link_libraries(echo_server_stream PRIVATE wasl)

macro(package_add_test_with_libraries TESTNAME FILES LIBRARIES TEST_WORKING_DIRECTORY)
  add_executable(${TESTNAME} ${FILES}
		${TEST_SCRIPTS_DIR}/sock_client.pl # TODO use findperl module
		)

  add_custom_command(
    TARGET ${TESTNAME}
    POST_BUILD
    COMMAND
    "${CMAKE_COMMAND}" -E copy_directory ${TEST_SCRIPTS_DIR} ${CMAKE_CURRENT_BINARY_DIR}/scripts
    )

  if (UNIX)
  target_link_libraries(${TESTNAME}
      PRIVATE
      Threads::Threads
      )

  endif()

  target_link_libraries(${TESTNAME} PRIVATE gtest_main ${LIBRARIES})

  gtest_discover_tests(${TESTNAME}
    WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  )

#  target_compile_features(${TESTNAME} PRIVATE cxx_std_14)
  set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()


cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)
cmake_print_variables(CMAKE_CURRENT_BINARY_DIR)
cmake_print_variables(testScripts)

add_custom_target(
  cp_test_scripts ALL
	COMMAND
  "${CMAKE_COMMAND}" -E copy_directory ${TEST_SCRIPTS_DIR} ${CMAKE_CURRENT_BINARY_DIR}/scripts
		)

# lib tests
package_add_test_with_libraries(vproxy_test VirtualProxy_test.cpp Wasl::Wasl "${PROJECT_DIR}")
#package_add_test_with_libraries(iomux_test IOMultiplexer_test.cpp wasl "${PROJECT_DIR}")
#package_add_test_with_libraries(socket_test Socket_test.cpp wasl "${PROJECT_DIR}")
#package_add_test_with_libraries(socketstream_test SocketStream_test.cpp wasl "${PROJECT_DIR}")

add_executable(socketstream_test SocketStream_test.cpp)
add_test(NAME sockstreamTest COMMAND socketstream_test)
target_link_libraries(socketstream_test PRIVATE gtest_main wasl)
gtest_discover_tests(socketstream_test)

add_executable(socket_test Socket_test.cpp)
add_test(NAME socketTest COMMAND socket_test)
target_link_libraries(socket_test PRIVATE gtest_main wasl)
gtest_discover_tests(socket_test)

add_executable(startServerTCP start_server_tcp_test.cpp)
target_link_libraries(startServerTCP PRIVATE wasl)
add_test(NAME initServerTCP COMMAND startServerTCP 127.0.0.1 9877)

add_executable(stream_client_test stream_client_test.cpp)
add_test(NAME streamClientTest COMMAND stream_client_test)
set_tests_properties(streamClientTest PROPERTIES LABELS "net")
target_link_libraries(stream_client_test PRIVATE gtest_main wasl)


set_tests_properties(initServerTCP PROPERTIES FIXTURES_SETUP	serverTCP)
set_tests_properties(streamClientTest PROPERTIES FIXTURES_REQUIRED serverTCP)
#set_tests_properties(stopServer			PROPERTIES FIXTURES_CLEANUP server)
