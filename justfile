default:
    @cd {{invocation_directory()}}; just --choose

mk-dir name:
    rm -rf {{ name }}-{{os()}}
    mkdir {{ name }}-{{os()}}

mk-build-valgrind: (mk-dir "build-valgrind")
    cd build-valgrind-{{os()}} ; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTESTS_WITH_LEAK_SANITIZER=NO

run-valgrind-no-slow:
    cd build-valgrind-{{os()}} ; ninja tests && valgrind --suppressions=../test/valgrind.supp ./test/tests '~[slow]'

mk-build-asan: (mk-dir "build-asan")
    cd build-asan-{{os()}} ; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTESTS_WITH_LEAK_SANITIZER=YES

run-tests-asan:
    cd build-asan-{{os()}} ; ninja tests && ./test/tests
