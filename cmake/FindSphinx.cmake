find_program(
	SPHINX_EXECUTABLE
	NAMES sphinx-build
	DOC "Path to sphinx-build executable")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	Sphinx "Unable to locate sphinx-build executable" SPHINX_EXECUTABLE)

