add_subdirectory(command)

add_library(claudecpp STATIC
    ${COMMAND_SOURCES}
    claudecpp.h
)

target_link_libraries(claudecpp
    PUBLIC
        Qt6::Core
)

target_include_directories(claudecpp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
