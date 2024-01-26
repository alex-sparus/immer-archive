[private]
default:
    @cd {{invocation_directory()}}; just --choose

_mk-dir name:
    rm -rf {{ name }}-{{os()}}
    mkdir {{ name }}-{{os()}}

# Create a build directory for a Debug build without ASAN, so that valgrind can work
mk-build-valgrind: (_mk-dir "build-valgrind")
    cd build-valgrind-{{os()}} ; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTESTS_WITH_LEAK_SANITIZER=NO

run-valgrind-no-slow:
    cd build-valgrind-{{os()}} ; ninja tests && valgrind --suppressions=../test/valgrind.supp ./test/tests '~[slow]'

# Create a build directory for a Debug build with ASAN enabled
mk-build-asan: (_mk-dir "build-asan")
    cd build-asan-{{os()}} ; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTESTS_WITH_LEAK_SANITIZER=YES

run-tests-asan:
    cd build-asan-{{os()}} ; ninja tests && ./test/tests
