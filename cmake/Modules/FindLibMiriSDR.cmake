if(NOT LIBMIRISDR_FOUND)

  pkg_check_modules (LIBMIRISDR_PKG libmirisdr)
  find_path(LIBMIRISDR_INCLUDE_DIR NAMES mirisdr.h
	PATHS
	${LIBMIRISDR_PKG_INCLUDE_DIRS}
	/usr/include
	/usr/local/include
  )

  find_library(LIBMIRISDR_LIBRARIES NAMES mirisdr
	PATHS
	${LIBMIRISDR_PKG_LIBRARY_DIRS}
	/usr/lib
	/usr/local/lib
  )

  if(LIBMIRISDR_INCLUDE_DIR AND LIBMIRISDR_LIBRARIES)
	set(LIBMIRISDR_FOUND TRUE CACHE INTERNAL "libmirisdr found")
	message(STATUS "Found libmirisdr: ${LIBMIRISDR_INCLUDE_DIR}, ${LIBMIRISDR_LIBRARIES}")
  else(LIBMIRISDR_INCLUDE_DIR AND LIBMIRISDR_LIBRARIES)
	set(LIBMIRISDR_FOUND FALSE CACHE INTERNAL "libmirisdr found")
	message(STATUS "libmirisdr not found.")
  endif(LIBMIRISDR_INCLUDE_DIR AND LIBMIRISDR_LIBRARIES)

  mark_as_advanced(LIBMIRISDR_INCLUDE_DIR LIBMIRISDR_LIBRARIES)

endif(NOT LIBMIRISDR_FOUND)
