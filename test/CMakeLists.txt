include(GNUInstallDirs)
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.10.0
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

macro(package_add_test_with_libraries TESTNAME FILES LIBRARIES TEST_WORKING_DIRECTORY)
  file(COPY ${TEST_SCRIPTS_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  add_executable(${TESTNAME} ${FILES})

  if (UNIX)
  target_link_libraries(${TESTNAME}
      PRIVATE
      Threads::Threads
      expect
      )

  endif()

  target_link_libraries(${TESTNAME} PRIVATE gtest_main gmock_main ${LIBRARIES})

  gtest_discover_tests(${TESTNAME}
    WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
  )

#  target_compile_features(${TESTNAME} PRIVATE cxx_std_14)
  set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

set(TEST_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
file(TO_NATIVE_PATH ${TEST_SCRIPTS_DIR} testScripts)
cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)
cmake_print_variables(CMAKE_CURRENT_BINARY_DIR)
cmake_print_variables(testScripts)


#add_custom_target(
#	copy_test_scripts ALL
#	COMMAND
#		file(COPY ${TEST_SCRIPTS_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#		)



# lib tests
package_add_test_with_libraries(vproxy_test VirtualProxy_test.cpp Wasl::Wasl "${PROJECT_DIR}")
#package_add_test_with_libraries(iomux_test IOMultiplexer_test.cpp wasl "${PROJECT_DIR}")
package_add_test_with_libraries(socket_test Socket_test.cpp wasl "${PROJECT_DIR}")
package_add_test_with_libraries(socketstream_test SocketStream_test.cpp wasl "${PROJECT_DIR}")

