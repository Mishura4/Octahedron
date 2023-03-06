add_executable(Octahedron)

source_group(
    TREE
        ${CMAKE_CURRENT_LIST_DIR}
    PREFIX
        "Source Files"
    FILES
        ${OCTAHEDRON_SOURCES}
)

target_sources(Octahedron
    PRIVATE
        ${OCTAHEDRON_SOURCES}
)

set_target_properties(Octahedron
    PROPERTIES
        LINKER_LANGUAGE CXX
)

target_compile_features(Octahedron PUBLIC cxx_std_20)

target_sources(Octahedron
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/stub.cpp
)