add_library(Octahedron)

set(OCTAHEDRON_HEADERS
        ${CMAKE_CURRENT_LIST_DIR}/octahedron.h
        ${CMAKE_CURRENT_LIST_DIR}/base.h
        ${CMAKE_CURRENT_LIST_DIR}/engine/game_engine.h
        ${CMAKE_CURRENT_LIST_DIR}/tools/protected_ptr.h
        ${CMAKE_CURRENT_LIST_DIR}/tools/math.h
        ${CMAKE_CURRENT_LIST_DIR}/tools/string_literal.h
        ${CMAKE_CURRENT_LIST_DIR}/tools/exception.h
        ${CMAKE_CURRENT_LIST_DIR}/tools/managed_resource.h
        ${CMAKE_CURRENT_LIST_DIR}/tools/tuple.h
        ${CMAKE_CURRENT_LIST_DIR}/io/logger.h
        ${CMAKE_CURRENT_LIST_DIR}/io/file_system.h
        ${CMAKE_CURRENT_LIST_DIR}/io/file_stream.h
        ${CMAKE_CURRENT_LIST_DIR}/io/io_interface.h
        ${CMAKE_CURRENT_LIST_DIR}/io/gz_file_stream.h
        ${CMAKE_CURRENT_LIST_DIR}/io/raw_file_stream.h
        ${CMAKE_CURRENT_LIST_DIR}/io/serializer.h
        ${CMAKE_CURRENT_LIST_DIR}/io/io_stream.h
        ${CMAKE_CURRENT_LIST_DIR}/io/io_interface.h
)

set(OCTAHEDRON_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/octahedron.cpp
        ${CMAKE_CURRENT_LIST_DIR}/engine/game_engine.cpp
        ${CMAKE_CURRENT_LIST_DIR}/io/file_system.cpp
        ${CMAKE_CURRENT_LIST_DIR}/io/io_stream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/io/file_stream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/io/gz_file_stream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/io/raw_file_stream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/io/dynamic_buffer.cpp
)

source_group(
    TREE
        ${CMAKE_CURRENT_LIST_DIR}
    PREFIX
        "Source Files"
    FILES
        ${OCTAHEDRON_HEADERS}
        ${OCTAHEDRON_SOURCES}
)

target_sources(Octahedron
    PUBLIC
        ${OCTAHEDRON_HEADERS}
    PRIVATE
        ${OCTAHEDRON_SOURCES}
 "io/utf8_stream.h" "io/utf8_stream.cpp" "io/encoding.h" "io/encoding.cpp")

set_target_properties(Octahedron
    PROPERTIES
        LINKER_LANGUAGE CXX
)

target_compile_features(Octahedron PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(Octahedron PUBLIC /Zc:preprocessor)
    target_compile_definitions(Octahedron
        PUBLIC
            _WIN32_WINNT=_WIN32_WINNT_WIN7
            WINVER=_WIN32_WINNT_WIN7)
endif()

#target_precompile_headers(Octahedron PRIVATE
#        ${CMAKE_CURRENT_LIST_DIR}/octahedron.h
#        ${CMAKE_CURRENT_LIST_DIR}/engine/engine.h
#        ${CMAKE_CURRENT_LIST_DIR}/tools/type_list.h
#        ${CMAKE_CURRENT_LIST_DIR}/tools/value_list.h
#        ${CMAKE_CURRENT_LIST_DIR}/tools/registry.h
#        ${CMAKE_CURRENT_LIST_DIR}/io/logger.h
#        ${CMAKE_CURRENT_LIST_DIR}/io/io_interface.h
#        ${CMAKE_CURRENT_LIST_DIR}/io/serializer.h
#        ${CMAKE_CURRENT_LIST_DIR}/io/io_interface.h
#)

target_include_directories(Octahedron PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(Octahedron PUBLIC ${ZSTR_DIR})

target_link_libraries(Octahedron PUBLIC SDL2-static)
target_link_libraries(Octahedron PUBLIC SDL2_image)
target_link_libraries(Octahedron PUBLIC SDL2_mixer)
target_link_libraries(Octahedron PUBLIC fmt)
target_link_libraries(Octahedron PUBLIC Boost::pfr)
target_link_libraries(Octahedron PUBLIC zlib)

if (WIN32)
    target_link_libraries(Octahedron PUBLIC SDL2::SDL2main)
endif ()
