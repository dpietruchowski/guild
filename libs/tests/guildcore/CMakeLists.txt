file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")

add_executable(guildcore_tests ${SOURCES} ${HEADERS})

target_link_libraries(guildcore_tests
    PRIVATE
        Qt6::Core
        Qt6::Test
        gtest
        guildcore
)

target_include_directories(guildcore_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/tmp)

target_compile_definitions(guildcore_tests
    PRIVATE
        GUILD_TEST_SCRATCH_DIR="${CMAKE_SOURCE_DIR}/tmp"
)

add_test(NAME guildcore_tests COMMAND guildcore_tests)
