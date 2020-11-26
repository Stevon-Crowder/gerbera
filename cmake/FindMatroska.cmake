# - Try to find libMatroska
# Once done this will define
#
#  Matroska_FOUND - system has libMatroska
#  Matroska_INCLUDE_DIRS - the libMatroska include directory
#  Matroska_LIBRARIES - Link these to use Matroska
#

INCLUDE(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

# Matroska
pkg_check_modules(PC_MAT QUIET libMatroska)
find_path(Matroska_INCLUDE_DIR KaxVersion.h
        HINTS ${PC_MAT_INCLUDEDIR} ${PC_MAT_INCLUDE_DIRS}
        PATH_SUFFIXES matroska)
find_library(Matroska_LIBRARY matroska
        HINTS ${PC_MAT_LIBDIR} ${PC_MAT_LIBRARY_DIRS})
find_package_handle_standard_args(Matroska
        REQUIRED_VARS Matroska_LIBRARY Matroska_INCLUDE_DIR)
if (Matroska_FOUND)
    set(Matroska_LIBRARIES ${Matroska_LIBRARY} ${PC_MAT_LIBRARIES})
    set(Matroska_INCLUDE_DIRS ${Matroska_INCLUDE_DIR})
endif ()
MARK_AS_ADVANCED(
        Matroska_INCLUDE_DIR
        Matroska_LIBRARY
)

