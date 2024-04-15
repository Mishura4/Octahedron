set(SDL2_DIR ${CMAKE_CURRENT_LIST_DIR}/SDL)

set(ENET_STATIC true)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/fmt)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/boost-pfr)

set(SDL2MIXER_MOD OFF)
set(SDL2MIXER_WAVPACK OFF)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SDL)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SDL_image)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SDL_mixer)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/zlib "${CMAKE_CURRENT_BINARY_DIR}/zlib_build")
set(ZSTR_DIR ${CMAKE_CURRENT_LIST_DIR}/zstr/src PARENT_SCOPE)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/enet)

message(STATUS ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(zlibstatic INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/zlib_build")
target_include_directories(zlibstatic INTERFACE ${CMAKE_CURRENT_LIST_DIR}/zlib)
target_include_directories(zlib INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/zlib_build")
target_include_directories(zlib INTERFACE ${CMAKE_CURRENT_LIST_DIR}/zlib)

target_include_directories(enet INTERFACE ${CMAKE_CURRENT_LIST_DIR}/enet/include)