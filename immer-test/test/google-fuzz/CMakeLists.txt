add_subdirectory($ENV{GOOGLE_FUZZTEST} fuzztest)

enable_testing()

include(GoogleTest)

add_executable(first_fuzz_test first_fuzz_test.cc)

# If you have other dependencies than FuzzTest, link them:
# target_link_libraries(first_fuzz_test PRIVATE other_deps)

link_fuzztest(first_fuzz_test)
gtest_discover_tests(first_fuzz_test)
