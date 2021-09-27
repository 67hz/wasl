find_package(Doxygen)
if (DOXYGEN_FOUND)
	set(DOXY_IN ${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in)
	set(DOXY_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

	configure_file(${DOXY_IN} ${DOXY_OUT} @ONLY)
		message(STATUS "Doxy docs: ${DOXY_IN}")

	add_custom_target(
	dox # ALL
	COMMAND ${DOXYGEN_EXECUTABLE} ${DOXY_OUT}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Generating Doxygen docs"
	VERBATIM
	)

	find_package(Sphinx)
	if (SPHINX_FOUND)
		set(SPHINX_IN ${CMAKE_CURRENT_SOURCE_DIR}/sphinx_conf.py.in)
	set(SPHINX_OUT ${CMAKE_CURRENT_BINARY_DIR}/conf.py)
	configure_file(${SPHINX_IN} ${SPHINX_OUT} @ONLY)
		add_custom_target(
		sphinx-doc ALL
		COMMAND ${SPHINX_EXECUTABLE} -b html -c ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating Sphinx docs"
		VERBATIM	)

	add_dependencies(sphinx-doc dox)
	endif(SPHINX_FOUND)

endif(DOXYGEN_FOUND)
