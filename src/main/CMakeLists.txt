add_library(Octahedron)

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

if (MSVC)
    target_compile_options(Octahedron PUBLIC /Zc:preprocessor)
    target_compile_definitions(Octahedron
        PUBLIC
            _WIN32_WINNT=_WIN32_WINNT_WIN7
            WINVER=_WIN32_WINNT_WIN7)
endif()
target_include_directories(Octahedron PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(Octahedron PUBLIC ${ZSTR_DIR})
target_link_libraries(Octahedron PUBLIC SDL2-static)
target_link_libraries(Octahedron PUBLIC SDL2_image)
target_link_libraries(Octahedron PUBLIC SDL2_mixer)
target_link_libraries(Octahedron PUBLIC fmt)
target_link_libraries(Octahedron PUBLIC Boost::pfr)

target_sources(Octahedron
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/Octahedron.h
        ${CMAKE_CURRENT_LIST_DIR}/Tools/ProtectedPtr.h
        ${CMAKE_CURRENT_LIST_DIR}/Engine/Engine.cpp
    PUBLIC
 "Tools/Math.h" "Tools/StringLiteral.h" "Tools/Exception.h" "Engine/Engine.h" "Tools/ManagedResource.h" "IO/Logger.h" "IO/FileSystem.h" "IO/FileSystem.cpp" "Octahedron.cpp" "Base.h" "IO/IOStream.h" "IO/IOStream.cpp" "IO/FileStream.cpp" "IO/FileStream.h" "IO/IOInterface.h")