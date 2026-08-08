cmake_minimum_required(VERSION 3.20)

#[[
ASE FOUNDATION BUILD CONFIGURATION

@file        CMakeLists.txt
@brief       Build configuration for ase-types
@description Types library - Result, Option, error handling primitives.
             Header-only library for type-safe error handling.

@module      ase-types
@layer       0 (Foundation)
@created     2025-12-15
@modified    2026-01-30
@version     00.00.05.00005 [poc]

CMAKE MODULE COMPLIANCE

[ ] cmake_minimum_required(VERSION 3.20) as FIRST line
[ ] project() with VERSION and LANGUAGES CXX
[ ] message(STATUS) for configuration output
[ ] add_library() with INTERFACE for header-only
[ ] add_library(ase::{module} ALIAS ...) for namespace
[ ] target_include_directories() with INTERFACE and generator expressions
[ ] target_compile_features() with cxx_std_20
[ ] NO external dependencies (Layer 0)
[ ] include(GNUInstallDirs) for installation
[ ] install(DIRECTORY include/ ...) for headers
]]

# Parse local VERSION (SSOT) if not building from root
if(NOT DEFINED ASE_TYPES_VERSION)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
        message(FATAL_ERROR "VERSION file not found: ${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    endif()
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _VERSION_LINES)
    foreach(_LINE ${_VERSION_LINES})
        if(_LINE MATCHES "^ASE_TYPES=(.*)$")
            set(ASE_TYPES_VERSION "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(NOT DEFINED ASE_TYPES_VERSION)
        message(FATAL_ERROR "ASE_TYPES version not found in VERSION file")
    endif()
endif()

project(ase-types VERSION ${ASE_TYPES_VERSION} LANGUAGES CXX)

# Generate version.hpp from template (SSOT)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ase/types/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/ase/types/version.hpp"
    @ONLY
)

# ==============================================================================
# BUILD CONFIGURATION
# ==============================================================================



message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")

#[[
HEADER-ONLY LIBRARY
Type primitives: Result<T,E>, Option<T>, error codes.
]]
add_library(${PROJECT_NAME} INTERFACE
    #[[
    CORE
    Main types header and entry point.
    ]]
    include/ase/types/types.hpp

    #[[
    RESULT
    Result<T,E> type for error handling without exceptions.
    ]]
    include/ase/types/result.hpp

    #[[
    OPTION
    Option<T> type for optional values without std::optional.
    ]]
    include/ase/types/option.hpp

    #[[
    MESH WIRE
    Frozen World-Mesh wire contract: BIN_MSG band 92-106, RegionRect + chunk_in_region,
    ChunkSnapEntry/PlayerSnap/EntitySnap byte layouts, mesh intent PODs. Shared by all
    four tiers so no module restates a frame id (PLAN_ASE_COMPUTE_PHASE_00_CONTRACT WS-0.1).
    ]]
    include/ase/types/region_wire.hpp

    #[[
    HEXGRID WIRE
    Frozen planet hex-lattice seam: the ONE chunk-id packing in its two forms (3D
    chunk_coords_to_id at 20 bits per axis for the height stack, 2D cell_to_chunk_id at 32 bits
    per axis for the lattice plane, plus chunk_id_to_cell_x / chunk_id_to_cell_z), the
    int32-through-f32 hub bit-pattern codec, and the cell-interaction / project-spawn intent PODs.
    Shared by World, Replica and Engine so no module restates the packing
    (PLAN_ASE_LATTICE_PHASE_00_CONTRACT WS-K.2b, WS-K.4; 2D width amended 2026-08-04 by
    PLAN_ASE_PRESSURE_PHASE_01_ADDR).
    ]]
    include/ase/types/hexgrid_wire.hpp
)
add_library(ase::types ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

#[[ VERBOTEN: No dependencies allowed in Layer 0! ]]

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(ASE_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()
