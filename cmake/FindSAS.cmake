# FindSAS.cmake - Locate SAS installation
#
# This module defines:
#  SAS_EXECUTABLE - Path to SAS executable
#  SAS_FOUND - True if SAS is found
#  SAS_VERSION - Version of SAS (if detectable)
#
# The module searches for SAS in common installation directories:
#  - Linux/Unix: /usr/local/SAS*, /opt/SAS*
#  - macOS: /Applications/SASHome
#  - Windows: C:/Program Files/SAS*, C:/SAS*
#
# Users can set SAS_PATH environment variable to override

# First check environment variable
if(DEFINED ENV{SAS_PATH})
    set(SAS_EXECUTABLE $ENV{SAS_PATH})
    if(EXISTS "${SAS_EXECUTABLE}")
        message(STATUS "Using SAS from SAS_PATH: ${SAS_EXECUTABLE}")
        set(SAS_FOUND TRUE)
        return()
    endif()
endif()

# Platform-specific search paths and executable names
if(UNIX AND NOT APPLE)
    # Linux/Unix
    set(SAS_SEARCH_PATHS
        "/usr/local/SASHome/SASFoundation/9.4/bin"
        "/usr/local/SAS/SASFoundation/9.4/bin"
        "/opt/SASHome/SASFoundation/9.4/bin"
        "/opt/SAS/SASFoundation/9.4/bin"
        "/usr/local/SASHome/SASFoundation/9.3/bin"
        "/usr/local/SAS94"
        "/usr/local/SAS93"
        "/opt/sasinside"
    )
    set(SAS_EXECUTABLE_NAMES sas sas.sh)

elseif(APPLE)
    # macOS
    set(SAS_SEARCH_PATHS
        "/Applications/SASHome/SASFoundation/9.4/sas.app/Contents/MacOS"
        "/Applications/SASHome/SASFoundation/9.4/bin"
        "/Applications/SASHome/SASFoundation/9.3/sas.app/Contents/MacOS"
        "/Applications/SASHome/SASFoundation/9.3/bin"
        "/Applications/SAS 9.4"
        "/Applications/SAS 9.3"
    )
    set(SAS_EXECUTABLE_NAMES sas sas_u8 sas_en)

elseif(WIN32)
    # Windows
    set(SAS_SEARCH_PATHS
        "C:/Program Files/SASHome/SASFoundation/9.4"
        "C:/Program Files/SASHome/SASFoundation/9.3"
        "C:/Program Files (x86)/SASHome/SASFoundation/9.4"
        "C:/Program Files (x86)/SASHome/SASFoundation/9.3"
        "C:/SAS/SASFoundation/9.4"
        "C:/SAS94"
        "C:/SAS93"
    )
    set(SAS_EXECUTABLE_NAMES sas.exe)
endif()

# Search for SAS executable
find_program(SAS_EXECUTABLE
    NAMES ${SAS_EXECUTABLE_NAMES}
    PATHS ${SAS_SEARCH_PATHS}
    PATH_SUFFIXES bin
    DOC "Path to SAS executable"
)

# Handle the standard arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SAS
    REQUIRED_VARS SAS_EXECUTABLE
)

# Try to detect SAS version (if found)
if(SAS_EXECUTABLE AND EXISTS "${SAS_EXECUTABLE}")
    # Try to extract version from path
    string(REGEX MATCH "9\\.[0-9]+" SAS_VERSION_FROM_PATH "${SAS_EXECUTABLE}")
    if(SAS_VERSION_FROM_PATH)
        set(SAS_VERSION ${SAS_VERSION_FROM_PATH})
        message(STATUS "Detected SAS version: ${SAS_VERSION}")
    endif()

    set(SAS_FOUND TRUE)
else()
    set(SAS_FOUND FALSE)
endif()

mark_as_advanced(SAS_EXECUTABLE SAS_VERSION)
