set_property(GLOBAL PROPERTY USE_FOLDERS ON)

SET(ENGINE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/engine/aa.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/bih.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/blend.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/client.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/command.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/console.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/dynlight.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/grass.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/light.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/main.cpp
    #${CMAKE_CURRENT_LIST_DIR}/engine/master.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/material.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/menus.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/movie.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/normal.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/octa.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/octaedit.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/octarender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/pch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/physics.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/pvs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/rendergl.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/renderlights.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/rendermodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/renderparticles.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/rendersky.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/rendertext.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/renderva.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/serverbrowser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/shader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/sound.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/stain.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/texture.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/ui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/water.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/world.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine/worldio.cpp
)

SET(ENGINE_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/engine/animmodel.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/bih.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/engine.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/explosion.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/hitzone.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/iqm.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/lensflare.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/light.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/lightning.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/md2.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/md3.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/md5.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/model.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/mpr.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/obj.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/octa.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/ragdoll.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/skelmodel.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/smd.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/textedit.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/texture.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/vertmodel.h
    ${CMAKE_CURRENT_LIST_DIR}/engine/world.h
)

set(SHARED_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/shared/crypto.cpp
    ${CMAKE_CURRENT_LIST_DIR}/shared/geom.cpp
    ${CMAKE_CURRENT_LIST_DIR}/shared/glemu.cpp
    ${CMAKE_CURRENT_LIST_DIR}/shared/pch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/shared/stream.cpp
    ${CMAKE_CURRENT_LIST_DIR}/shared/tools.cpp
    ${CMAKE_CURRENT_LIST_DIR}/shared/zip.cpp
)

set(SHARED_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/shared/command.h
    ${CMAKE_CURRENT_LIST_DIR}/shared/cube.h
    ${CMAKE_CURRENT_LIST_DIR}/shared/ents.h
    ${CMAKE_CURRENT_LIST_DIR}/shared/geom.h
    ${CMAKE_CURRENT_LIST_DIR}/shared/glemu.h
    ${CMAKE_CURRENT_LIST_DIR}/shared/glexts.h
    ${CMAKE_CURRENT_LIST_DIR}/shared/iengine.h
    ${CMAKE_CURRENT_LIST_DIR}/shared/igame.h
    ${CMAKE_CURRENT_LIST_DIR}/shared/tools.h
)

set(GAME_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/game/ai.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game/client.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game/entities.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game/game.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game/pch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game/render.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game/scoreboard.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game/server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game/waypoint.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game/weapon.cpp
)

set(GAME_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/game/ai.h
    ${CMAKE_CURRENT_LIST_DIR}/game/aiman.h
    ${CMAKE_CURRENT_LIST_DIR}/game/ctf.h
    ${CMAKE_CURRENT_LIST_DIR}/game/extinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/game/game.h
)
add_executable(penteract
    WIN32
    ${ENGINE_HEADERS}
    ${SHARED_HEADERS}
    ${GAME_HEADERS}
    ${ENGINE_SOURCES}
    ${SHARED_SOURCES}
    ${GAME_SOURCES}
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

set_target_properties(penteract
    PROPERTIES
        LINKER_LANGUAGE CXX
)

find_package(OpenGL REQUIRED)

target_compile_features(penteract PUBLIC cxx_std_20)

target_include_directories(penteract PRIVATE ${CMAKE_CURRENT_LIST_DIR}/engine)
target_include_directories(penteract PRIVATE ${CMAKE_CURRENT_LIST_DIR}/game)
target_include_directories(penteract PRIVATE ${CMAKE_CURRENT_LIST_DIR}/shared)

# target_link_libraries(Octahedron PUBLIC fmt)
target_link_libraries(penteract PUBLIC Octahedron)
target_link_libraries(penteract PUBLIC enet)
target_link_libraries(penteract PUBLIC zlib)
target_link_libraries(penteract PUBLIC ${OPENGL_LIBRARIES})

#target_compile_options(penteract PUBLIC $<$<CONFIG:RelWithDebInfo>:-fsanitize=address>)
#target_link_options(penteract PUBLIC <$<CONFIG:RelWithDebInfo>:-fsanitize=address>)


if(WIN32)
  target_link_libraries(penteract PUBLIC wsock32 ws2_32)
endif()

add_custom_command(TARGET penteract POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_RUNTIME_DLLS:penteract>" "$<TARGET_FILE_DIR:penteract>"
  COMMAND_EXPAND_LISTS
)

set_target_properties(penteract
    PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VS_DEBUGGER_COMMAND_ARGUMENTS "-urun -glog.txt"
)