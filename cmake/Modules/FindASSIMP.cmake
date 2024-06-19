find_path(ASSIMP_INCLUDE_DIR assimp/scene.h
  PATHS /usr/include
  DOC "The directory where assimp/scene.h resides")

find_library(ASSIMP_LIBRARY
  NAMES assimp
  PATHS /usr/lib/aarch64-linux-gnu
  DOC "The Assimp library")

find_package(assimp QUIET CONFIG PATHS ${CMAKE_PREFIX_PATH} ${CMAKE_SYSTEM_PREFIX_PATH})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Assimp DEFAULT_MSG
  ASSIMP_INCLUDE_DIR
  ASSIMP_LIBRARY
  ASSIMP_CMAKE_CONFIG_DIR)

if(ASSIMP_FOUND AND NOT TARGET Assimp::Assimp)
  add_library(Assimp::Assimp SHARED IMPORTED)
  set_target_properties(Assimp::Assimp PROPERTIES
    IMPORTED_LOCATION "${ASSIMP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${ASSIMP_INCLUDE_DIR}")
endif()