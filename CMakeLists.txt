cmake_minimum_required(VERSION 3.13.4)
project(wasl VERSION 0.2.0 LANGUAGES CXX)

include(CMakePrintHelpers)
include(FetchContent)

#set(CMAKE_CXX_CLANG_TIDY clang-tidy --header-filter='[^gtest*]' -checks=-*,clang-analyzer-*,clang-analyzer-cplusplus-*,cppcoreguidelines-*,llvm-*)

cmake_print_variables(SRC_DIR)
cmake_print_variables(PROJECT_SOURCE_DIR)
cmake_print_variables(CMAKE_CURRENT_LIST_DIR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

FetchContent_Declare(GSL
	GIT_REPOSITORY "https://github.com/microsoft/GSL"
	GIT_TAG "v3.1.0"
)
FetchContent_MakeAvailable(GSL)

if (WIN32)
  if (BUILD_SHARED_LIBS)
	  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES CACHE BOOL "Export all symbols")
  endif()
endif()

set(path_to_headers
	"${CMAKE_CURRENT_LIST_DIR}/include"
)

file(GLOB wasl_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB wasl_headers CONFIGURE_DEPENDS "${path_to_headers}/wasl/*.h")


cmake_print_variables(wasl_sources)
cmake_print_variables(wasl_headers)

add_library(wasl ${wasl_sources})
add_library(Wasl::Wasl ALIAS wasl)

target_link_libraries(wasl PUBLIC GSL)

target_include_directories(wasl
  PUBLIC
  $<BUILD_INTERFACE:${path_to_headers}>

  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/>

  PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_definitions(wasl PRIVATE
  _LARGEFILE_SOURCE _FILE_OFFSET_BITS=64 _CRT_SECURE_NO_DEPRECATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE _CRT_NONSTDC_NO_WARNINGS _SCL_SECURE_NO_DEPRECATE _SCL_SECURE_NO_WARNINGS UNICODE _UNICODE __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS __STDC_LIMIT_MACROS NOMINMAX)

target_compile_features(wasl PRIVATE cxx_std_14)

cmake_print_variables(CMAKE_CXX_COMPILER_ID)
target_compile_options(wasl
  PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
  -Wall -Wextra -fdiagnostics-color=always>
  $<$<CXX_COMPILER_ID:MSVC>:
   /W4>)


include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Wasl)

install(TARGETS wasl
  EXPORT wasl-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_DESTINATION_DIR}
)

set_target_properties(wasl PROPERTIES EXPORT_NAME Wasl)


# Export namespaced targets to a generated cmake script
install(EXPORT wasl-targets
    FILE
      WaslTargets.cmake
    NAMESPACE
      Wasl::
    DESTINATION
      ${INSTALL_CONFIGDIR}
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/WaslConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/WaslConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/WaslConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

export(EXPORT wasl-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/WaslTargets.cmake
    NAMESPACE Wasl::)

# Register package in the User Package Registry
export(PACKAGE Wasl)

option(BUILD_TESTING "Build unit tests" ON)
if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)
endif()

# todo use generate $<IF:CONFIG
if (CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
	find_program(CLANG_TIDY "clang-tidy")
	if (CLANG_TIDY)
	add_custom_target(
			clang-tidy ALL
			COMMAND ${CMAKE_CXX_CLANG_TIDY}
			${wasl_sources}
			${wasl_headers}
		)
	endif()

	find_program(CLANG_FORMAT "clang-format")
	if (CLANG_FORMAT)
		add_custom_target(
			clang-format ALL
			COMMAND /usr/bin/clang-format -style=LLVM -i ${wasl_sources} ${wasl_headers}
		)
	endif()

endif()

option (BUILD_DOC "Build the docs" ON)
if (BUILD_DOC)
	add_subdirectory(docs)
endif()

