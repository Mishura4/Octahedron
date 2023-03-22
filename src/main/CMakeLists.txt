add_library(Octahedron)

set(OCTAHEDRON_HEADERS
        ${CMAKE_CURRENT_LIST_DIR}/Octahedron.h
        ${CMAKE_CURRENT_LIST_DIR}/Base.h
        ${CMAKE_CURRENT_LIST_DIR}/Engine/Engine.h
        ${CMAKE_CURRENT_LIST_DIR}/Tools/ProtectedPtr.h
        ${CMAKE_CURRENT_LIST_DIR}/Tools/Math.h
        ${CMAKE_CURRENT_LIST_DIR}/Tools/StringLiteral.h
        ${CMAKE_CURRENT_LIST_DIR}/Tools/Exception.h
        ${CMAKE_CURRENT_LIST_DIR}/Tools/ManagedResource.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/Logger.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/FileSystem.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/FileStream.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/IOInterface.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/GZFileStream.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/RawFileStream.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/Serializer.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/IOStream.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/IOInterface.h
)

set(OCTAHEDRON_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/Octahedron.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Engine/Engine.cpp
        ${CMAKE_CURRENT_LIST_DIR}/IO/FileSystem.cpp
        ${CMAKE_CURRENT_LIST_DIR}/IO/IOStream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/IO/FileStream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/IO/GZFileStream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/IO/RawFileStream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/IO/DynamicBuffer.cpp
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
)

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

target_precompile_headers(Octahedron PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/Octahedron.h
        ${CMAKE_CURRENT_LIST_DIR}/Engine/Engine.h
        ${CMAKE_CURRENT_LIST_DIR}/Tools/TypeList.h
        ${CMAKE_CURRENT_LIST_DIR}/Tools/ValueList.h
        ${CMAKE_CURRENT_LIST_DIR}/Tools/Registry.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/Logger.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/IOInterface.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/Serializer.h
        ${CMAKE_CURRENT_LIST_DIR}/IO/IOInterface.h
)

target_include_directories(Octahedron PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(Octahedron PUBLIC ${ZSTR_DIR})
target_link_libraries(Octahedron PUBLIC SDL2-static)
target_link_libraries(Octahedron PUBLIC SDL2_image)
target_link_libraries(Octahedron PUBLIC SDL2_mixer)
target_link_libraries(Octahedron PUBLIC fmt)
target_link_libraries(Octahedron PUBLIC Boost::pfr)
target_link_libraries(Octahedron PUBLIC zlib)