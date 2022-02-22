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
include(CTest)
enable_testing()

set(TEST_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
file(TO_NATIVE_PATH ${TEST_SCRIPTS_DIR} testScripts)
file(GLOB test_helpers CONFIGURE_DEPENDS "${TEST_SCRIPTS_DIR}/*.pl")
cmake_print_variables(test_helpers)

macro(package_add_test_with_libraries TESTNAME FILES LIBRARIES TEST_WORKING_DIRECTORY)
  add_executable(${TESTNAME} ${FILES} ${test_helpers})

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
package_add_test_with_libraries(socketstream_test SocketStream_test.cpp wasl "${PROJECT_DIR}")

add_executable(socket_test Socket_test.cpp)
add_test(NAME socketTest COMMAND socket_test)
set_tests_properties(socketTest PROPERTIES LABELS "net")
target_link_libraries(socket_test PRIVATE gtest_main wasl)
gtest_discover_tests(socket_test)


add_executable(start_tcp_server start_server_tcp.cpp)
add_test(NAME startTCPServer COMMAND start_tcp_server)
target_link_libraries(start_tcp_server PRIVATE gtest_main wasl)


set_tests_properties(startTCPServer		PROPERTIES FIXTURES_SETUP	server)
set_tests_properties(socketTest PROPERTIES FIXTURES_REQUIRED server)
#set_tests_properties(stopServer			PROPERTIES FIXTURES_CLEANUP server)






