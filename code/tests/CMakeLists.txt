include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
    SYSTEM
)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TESTS_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_executable(tests ${TESTS_SOURCES})
target_link_libraries(tests PRIVATE project_compilation_options forwarding_tables GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests NO_PRETTY_TYPES)
