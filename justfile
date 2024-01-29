[private]
default:
    @cd {{ invocation_directory() }}; just --choose

_mk-dir name:
    rm -rf {{ name }}
    mkdir {{ name }}

build-valgrind-path := "build-valgrind-" + os() + "-" + arch()

# Create a build directory for a Debug build without ASAN, so that valgrind can work
[linux]
mk-build-valgrind: (_mk-dir build-valgrind-path)
    cd {{ build-valgrind-path }} ; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTESTS_WITH_LEAK_SANITIZER=NO

[linux]
run-valgrind-no-slow:
    cd {{ build-valgrind-path }} ; ninja tests && valgrind --suppressions=../test/valgrind.supp ./test/tests '~[slow]'

build-asan-path := "build-asan-" + os() + "-" + arch()

# Create a build directory for a Debug build with ASAN enabled
mk-build-asan: (_mk-dir build-asan-path)
    cd {{ build-asan-path }} ; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTESTS_WITH_LEAK_SANITIZER=YES

run-tests-asan:
    cd {{ build-asan-path }} ; ninja tests && ./test/tests
