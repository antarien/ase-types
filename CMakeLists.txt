cmake_minimum_required(VERSION 3.20)

project(ase-types
    VERSION 0.1.0
    DESCRIPTION "ASE Types Library - Result, Option"
    LANGUAGES CXX
)

# Header-only library
add_library(ase-types INTERFACE)
add_library(ase::types ALIAS ase-types)

target_include_directories(ase-types
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(ase-types INTERFACE cxx_std_20)

# Install
install(TARGETS ase-types
    EXPORT ase-types-targets
)

install(DIRECTORY include/ase
    DESTINATION include
)
