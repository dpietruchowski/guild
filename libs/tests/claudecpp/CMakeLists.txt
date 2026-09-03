file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")

add_executable(claudecpp_tests ${SOURCES} ${HEADERS})

target_link_libraries(claudecpp_tests
    PRIVATE
        Qt6::Core
        Qt6::Test
        gtest
        claudecpp
)

add_test(NAME claudecpp_tests COMMAND claudecpp_tests)
