cmake_minimum_required(VERSION 3.21)

project(Octahedron)

set(SDL2IMAGE_INSTALL OFF)
set(BUILD_SHARED_LIBS FALSE)

add_subdirectory(dep/)

add_subdirectory(src/main Octahedron)
add_subdirectory(src/tests tests)
add_subdirectory(src/penteract penteract)

#target_link_libraries(Octahedron PUBLIC boost_pfr)
#target_link_libraries(Octahedron PUBLIC magic_enum)