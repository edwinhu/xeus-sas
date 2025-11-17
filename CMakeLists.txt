cmake_minimum_required(VERSION 3.8)
project(xeus-sas VERSION 0.1.0)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_DOCS "Build documentation" OFF)

# Find dependencies
find_package(xeus 5.0 REQUIRED)
find_package(xeus-zmq 3.0 REQUIRED)
find_package(xtl 0.7 REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)
find_package(Threads REQUIRED)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find SAS (optional - for setting default path)
include(FindSAS)

if(SAS_EXECUTABLE)
    message(STATUS "Found SAS: ${SAS_EXECUTABLE}")
    add_definitions(-DDEFAULT_SAS_PATH="${SAS_EXECUTABLE}")
else()
    message(WARNING "SAS not found. Kernel will require SAS_PATH environment variable.")
endif()

# Source files
set(XEUS_SAS_SRC
    src/main.cpp
    src/xinterpreter.cpp
    src/sas_session.cpp
    src/sas_parser.cpp
    src/completion.cpp
    src/inspection.cpp
)

set(XEUS_SAS_HEADERS
    include/xeus-sas/xeus_sas_config.hpp
    include/xeus-sas/xinterpreter.hpp
    include/xeus-sas/sas_session.hpp
    include/xeus-sas/sas_parser.hpp
    include/xeus-sas/completion.hpp
    include/xeus-sas/inspection.hpp
)

# Executable
add_executable(xsas ${XEUS_SAS_SRC} ${XEUS_SAS_HEADERS})

# Link libraries
target_link_libraries(xsas
    PRIVATE
        xeus
        xeus-zmq
        nlohmann_json::nlohmann_json
        Threads::Threads
)

# Include directories
target_include_directories(xsas
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(xsas PRIVATE -Wall -Wextra -pedantic)
elseif(MSVC)
    target_compile_options(xsas PRIVATE /W4)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS xsas
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install kernel spec
set(KERNELSPEC_DIR ${CMAKE_INSTALL_PREFIX}/share/jupyter/kernels/xeus-sas)
configure_file(
    share/jupyter/kernels/xeus-sas/kernel.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/kernel.json
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kernel.json
    DESTINATION ${KERNELSPEC_DIR}
)

# Install logos (if they exist)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xeus-sas/logo-32x32.png")
    install(FILES
        share/jupyter/kernels/xeus-sas/logo-32x32.png
        share/jupyter/kernels/xeus-sas/logo-64x64.png
        DESTINATION ${KERNELSPEC_DIR}
    )
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Documentation
if(BUILD_DOCS)
    add_subdirectory(docs)
endif()
