add_executable(ase-types-test
    hexgrid_wire_test.cpp
)

#[[
The doctest entry point is switched on here rather than through a define in the test source, so
the test file stays free of macro definitions.
]]
target_compile_definitions(ase-types-test
    PRIVATE
        DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
)

target_link_libraries(ase-types-test
    PRIVATE
        ase::types
        doctest::doctest
)

add_test(NAME ase-types-test COMMAND ase-types-test)

set_target_properties(ase-types-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
