#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "swiftwire::swiftwire" for configuration ""
set_property(TARGET swiftwire::swiftwire APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(swiftwire::swiftwire PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libswiftwire.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS swiftwire::swiftwire )
list(APPEND _IMPORT_CHECK_FILES_FOR_swiftwire::swiftwire "${_IMPORT_PREFIX}/lib/libswiftwire.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
