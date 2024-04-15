set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_executable(tests
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/io/ring.cpp
    ${CMAKE_CURRENT_LIST_DIR}/io/file_stream.cpp
)

message(WARNING ${ENGINE_HEADERS})

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}
    FILES
        ${ENGINE_HEADERS}
        ${SHARED_HEADERS}
        ${GAME_HEADERS}
        ${ENGINE_SOURCES}
        ${SHARED_SOURCES}
        ${GAME_SOURCES}
)

set_target_properties(tests
    PROPERTIES
        LINKER_LANGUAGE CXX
)

find_package(OpenGL REQUIRED)

target_compile_features(tests PUBLIC cxx_std_20)

target_include_directories(tests PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# target_link_libraries(Octahedron PUBLIC fmt)
target_link_libraries(tests PUBLIC Octahedron)
target_link_libraries(tests PUBLIC enet)
target_link_libraries(tests PUBLIC zlib)
target_link_libraries(tests PUBLIC fmt)
target_link_libraries(tests PUBLIC ${OPENGL_LIBRARIES})

target_link_libraries(tests PUBLIC penteract_lib)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/testdata DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#target_compile_options(tests PUBLIC $<$<CONFIG:RelWithDebInfo>:-fsanitize=address>)
#target_link_options(tests PUBLIC <$<CONFIG:RelWithDebInfo>:-fsanitize=address>)


if(WIN32)
  target_link_libraries(tests PUBLIC wsock32 ws2_32)
endif()

add_custom_command(TARGET tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_RUNTIME_DLLS:tests>" "$<TARGET_FILE_DIR:tests>"
  COMMAND_EXPAND_LISTS
)
