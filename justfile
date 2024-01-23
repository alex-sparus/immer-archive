default:
    @just --choose

mk-dir name:
    rm -rf {{ name }}
    mkdir {{ name }}

mk-build-valgrind: (mk-dir "build-valgrind")
    cd build-valgrind ; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTESTS_WITH_LEAK_SANITIZER=NO

valgrind-no-slow:
    cd {{invocation_directory()}}; ninja tests && valgrind --suppressions=../test/valgrind.supp ./test/tests '~[valgrind]' '~[slow]'
