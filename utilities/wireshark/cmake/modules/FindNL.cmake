#
# Find the native netlink includes and library
#
# If they exist, differentiate between versions 1, 2 and 3.
# Version 1 does not have netlink/version.h
# Version 3 does have the major version number as a suffix
#   to the libnl name (libnl-3)
#
#  NL_INCLUDE_DIRS - where to find libnl.h, etc.
#  NL_LIBRARIES    - List of libraries when using libnl3.
#  NL_FOUND        - True if libnl found.

IF (NL_LIBRARIES AND NL_INCLUDE_DIRS )
  # in cache already
  SET(NL_FOUND TRUE)
ELSE (NL_LIBRARIES AND NL_INCLUDE_DIRS )
  SET( SEARCHPATHS
      /opt/local
      /sw
      /usr
      /usr/local
  )

  find_package(PkgConfig)
  pkg_check_modules(NL3 libnl-3.0 libnl-genl-3.0 libnl-route-3.0)
  pkg_search_module(NL2 libnl-2.0)

  FIND_PATH( NL_INCLUDE_DIR
    PATH_SUFFIXES
      include/libnl3
    NAMES
      netlink/version.h
    HINTS
      "${NL3_libnl-3.0_INCLUDEDIR}"
      "${NL2_INCLUDEDIR}"
    PATHS
      $(SEARCHPATHS)
  )
  # NL version >= 2
  IF ( NL_INCLUDE_DIR )
    FIND_LIBRARY( NL_LIBRARY
      NAMES
        nl-3 nl
      PATH_SUFFIXES
        lib64 lib
      HINTS
        "${NL3_libnl-3.0_LIBDIR}"
        "${NL2_LIBDIR}"
      PATHS
        $(SEARCHPATHS)
    )
    FIND_LIBRARY( NLGENL_LIBRARY
      NAMES
        nl-genl-3 nl-genl
      PATH_SUFFIXES
        lib64 lib
      HINTS
        "${NL3_libnl-genl-3.0_LIBDIR}"
        "${NL2_LIBDIR}"
      PATHS
        $(SEARCHPATHS)
    )
    FIND_LIBRARY( NLROUTE_LIBRARY
      NAMES
        nl-route-3 nl-route
      PATH_SUFFIXES
        lib64 lib
      HINTS
        "${NL3_libnl-route-3.0_LIBDIR}"
        "${NL2_LIBDIR}"
      PATHS
        $(SEARCHPATHS)
    )
    #
    # If we don't have all of those libraries, we can't use libnl.
    #
    IF ( NOT NLGENL_LIBRARY AND NOT NLROUTE_LIBRARY )
      SET( NL_LIBRARY NOTFOUND )
    ENDIF ( NOT NLGENL_LIBRARY AND NOT NLROUTE_LIBRARY )
    IF( NL_LIBRARY )
      STRING(REGEX REPLACE ".*nl-([^.,;]*).*" "\\1" NLSUFFIX ${NL_LIBRARY})
      IF ( NLSUFFIX )
        SET( HAVE_LIBNL3 1 )
      ELSE ( NLSUFFIX )
        SET( HAVE_LIBNL2 1 )
      ENDIF (NLSUFFIX )
      SET( HAVE_LIBNL 1 )
    ENDIF( NL_LIBRARY )
  ELSE( NL_INCLUDE_DIR )
    # NL version 1 ?
    pkg_search_module(NL1 libnl-1)
    FIND_PATH( NL_INCLUDE_DIR
      NAMES
        netlink/netlink.h
      HINTS
        "${NL1_INCLUDEDIR}"
      PATHS
        $(SEARCHPATHS)
    )
    FIND_LIBRARY( NL_LIBRARY
      NAMES
        nl
      PATH_SUFFIXES
        lib64 lib
      HINTS
        "${NL1_LIBDIR}"
      PATHS
        $(SEARCHPATHS)
    )
    if ( NL_INCLUDE_DIR )
      SET( HAVE_LIBNL1 1 )
    ENDIF ( NL_INCLUDE_DIR )
  ENDIF( NL_INCLUDE_DIR )
ENDIF(NL_LIBRARIES AND NL_INCLUDE_DIRS)
# MESSAGE(STATUS "LIB Found: ${NL_LIBRARY}, Suffix: ${NLSUFFIX}\n  1:${HAVE_LIBNL1}, 2:${HAVE_LIBNL2}, 3:${HAVE_LIBNL3}.")

# handle the QUIETLY and REQUIRED arguments and set NL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NL DEFAULT_MSG NL_LIBRARY NL_INCLUDE_DIR)

IF(NL_FOUND)
  SET( NL_LIBRARIES ${NLGENL_LIBRARY} ${NLROUTE_LIBRARY} ${NL_LIBRARY} )
  SET( NL_INCLUDE_DIRS ${NL_INCLUDE_DIR})
ELSE()
  SET( NL_LIBRARIES )
  SET( NL_INCLUDE_DIRS )
ENDIF()

MARK_AS_ADVANCED( NL_LIBRARIES NL_INCLUDE_DIRS )

