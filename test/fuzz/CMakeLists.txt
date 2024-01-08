function(add_fuzzer bin_name file)
  add_executable(${bin_name} ${file})
  target_include_directories(${bin_name} PRIVATE ../..)
  target_compile_options(
    ${bin_name} PRIVATE -g -O1 -fno-optimize-sibling-calls
                        -fno-omit-frame-pointer -fsanitize=fuzzer,address)
  target_link_libraries(${bin_name} PRIVATE spdlog::spdlog)
  target_link_options(${bin_name} PRIVATE -g -O1 -fsanitize=fuzzer,address)
  target_compile_options(
    ${bin_name}
    PRIVATE -Wno-unknown-warning-option
            -Werror
            -Wall
            -Wextra
            -pedantic
            -Wno-unused-parameter
            -Wno-c++20-designator
            -Wno-gnu-anonymous-struct
            -Wno-nested-anon-types
            -Wno-unused-variable
            # -Wno-nullability-extension -Wno-deprecated-declarations
  )
endfunction()

add_fuzzer(fuzz_set fuzz_set.cpp)
add_fuzzer(flex-vector flex-vector.cpp)
