add_subdirectory(agent)
add_subdirectory(workspace)

add_library(guildcore STATIC
    ${AGENT_SOURCES}
    ${WORKSPACE_SOURCES}
    guild.h
    guild.cpp
    guildcore.h
)

target_link_libraries(guildcore
    PUBLIC
        Qt6::Core
        claudecpp
)

target_include_directories(guildcore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
